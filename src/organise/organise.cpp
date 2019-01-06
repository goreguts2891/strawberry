/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <functional>

#include <QtGlobal>
#include <QThread>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QString>
#include <QStringBuilder>
#include <QUrl>
#include <QtDebug>

#include "core/logging.h"
#include "core/utilities.h"
#include "core/taskmanager.h"
#include "core/musicstorage.h"
#include "organise.h"
#ifdef HAVE_GSTREAMER
#  include "transcoder/transcoder.h"
#endif

class OrganiseFormat;

using std::placeholders::_1;

const int Organise::kBatchSize = 10;
#ifdef HAVE_GSTREAMER
const int Organise::kTranscodeProgressInterval = 500;
#endif

Organise::Organise(TaskManager *task_manager, std::shared_ptr<MusicStorage> destination, const OrganiseFormat &format, bool copy, bool overwrite, bool mark_as_listened, const NewSongInfoList &songs_info, bool eject_after)
    : thread_(nullptr),
      task_manager_(task_manager),
#ifdef HAVE_GSTREAMER
      transcoder_(new Transcoder(this)),
#endif
      destination_(destination),
      format_(format),
      copy_(copy),
      overwrite_(overwrite),
      mark_as_listened_(mark_as_listened),
      eject_after_(eject_after),
      task_count_(songs_info.count()),
#ifdef HAVE_GSTREAMER
      transcode_suffix_(1),
#endif
      tasks_complete_(0),
      started_(false),
      task_id_(0),
      current_copy_progress_(0) {

  original_thread_ = thread();

  for (const NewSongInfo &song_info : songs_info) {
    tasks_pending_ << Task(song_info);
  }

}

void Organise::Start() {

  if (thread_) return;

  task_id_ = task_manager_->StartTask(tr("Organising files"));
  task_manager_->SetTaskBlocksCollectionScans(true);

  thread_ = new QThread;
  connect(thread_, SIGNAL(started()), SLOT(ProcessSomeFiles()));
#ifdef HAVE_GSTREAMER
  connect(transcoder_, SIGNAL(JobComplete(QString, QString, bool)), SLOT(FileTranscoded(QString, QString, bool)));
#endif

  moveToThread(thread_);
  thread_->start();
}

void Organise::ProcessSomeFiles() {

  if (!started_) {
#ifdef HAVE_GSTREAMER
    transcode_temp_name_.open();
#endif

    if (!destination_->StartCopy(&supported_filetypes_)) {
      // Failed to start - mark everything as failed :(
      for (const Task &task : tasks_pending_) files_with_errors_ << task.song_info_.song_.url().toLocalFile();
      tasks_pending_.clear();
    }
    started_ = true;
  }

  // None left?
  if (tasks_pending_.isEmpty()) {
#ifdef HAVE_GSTREAMER
    if (!tasks_transcoding_.isEmpty()) {
      // Just wait - FileTranscoded will start us off again in a little while
      qLog(Debug) << "Waiting for transcoding jobs";
      transcode_progress_timer_.start(kTranscodeProgressInterval, this);
      return;
    }
#endif

    UpdateProgress();

    destination_->FinishCopy(files_with_errors_.isEmpty());
    if (eject_after_) destination_->Eject();

    task_manager_->SetTaskFinished(task_id_);

    emit Finished(files_with_errors_);

    // Move back to the original thread so deleteLater() can get called in the main thread's event loop
    moveToThread(original_thread_);
    deleteLater();

    // Stop this thread
    thread_->quit();
    return;
  }

  // We process files in batches so we can be cancelled part-way through.
  for (int i = 0; i < kBatchSize; ++i) {
    SetSongProgress(0);

    if (tasks_pending_.isEmpty()) break;

    Task task = tasks_pending_.takeFirst();
    qLog(Info) << "Processing" << task.song_info_.song_.url().toLocalFile();

    // Use a Song instead of a tag reader
    Song song = task.song_info_.song_;
    if (!song.is_valid()) continue;

#ifdef HAVE_GSTREAMER
    // Maybe this file is one that's been transcoded already?
    if (!task.transcoded_filename_.isEmpty()) {
      qLog(Debug) << "This file has already been transcoded";

      // Set the new filetype on the song so the formatter gets it right
      song.set_filetype(task.new_filetype_);

      // Fiddle the filename extension as well to match the new type
      song.set_url(QUrl::fromLocalFile(Utilities::FiddleFileExtension(song.basefilename(), task.new_extension_)));
      song.set_basefilename(Utilities::FiddleFileExtension(song.basefilename(), task.new_extension_));

      // Have to set this to the size of the new file or else funny stuff happens
      song.set_filesize(QFileInfo(task.transcoded_filename_).size());
    }
    else {
      // Figure out if we need to transcode it
      Song::FileType dest_type = CheckTranscode(song.filetype());
      if (dest_type != Song::FileType_Unknown) {
        // Get the preset
        TranscoderPreset preset = Transcoder::PresetForFileType(dest_type);
        qLog(Debug) << "Transcoding with" << preset.name_;

        // Get a temporary name for the transcoded file
        task.transcoded_filename_ = transcode_temp_name_.fileName() + "-" + QString::number(transcode_suffix_++);
        task.new_extension_ = preset.extension_;
        task.new_filetype_ = dest_type;
        tasks_transcoding_[task.song_info_.song_.url().toLocalFile()] = task;

        qLog(Debug) << "Transcoding to" << task.transcoded_filename_;

        // Start the transcoding - this will happen in the background and FileTranscoded() will get called when it's done.
	// At that point the task will get re-added to the pending queue with the new filename.
        transcoder_->AddJob(task.song_info_.song_.url().toLocalFile(), preset, task.transcoded_filename_);
        transcoder_->Start();
        continue;
      }
    }
#endif

    MusicStorage::CopyJob job;
    job.source_ = task.transcoded_filename_.isEmpty() ? task.song_info_.song_.url().toLocalFile() : task.transcoded_filename_;
    job.destination_ = task.song_info_.new_filename_;
    job.metadata_ = song;
    job.overwrite_ = overwrite_;
    job.mark_as_listened_ = mark_as_listened_;
    job.remove_original_ = !copy_;
    job.progress_ = std::bind(&Organise::SetSongProgress, this, _1, !task.transcoded_filename_.isEmpty());

    if (!destination_->CopyToStorage(job)) {
      files_with_errors_ << task.song_info_.song_.basefilename();
    } else {
      if (job.mark_as_listened_) {
        emit FileCopied(job.metadata_.id());
      }
    }

    // Clean up the temporary transcoded file
    if (!task.transcoded_filename_.isEmpty())
      QFile::remove(task.transcoded_filename_);

    tasks_complete_++;
  }
  SetSongProgress(0);

  QTimer::singleShot(0, this, SLOT(ProcessSomeFiles()));

}

#ifdef HAVE_GSTREAMER
Song::FileType Organise::CheckTranscode(Song::FileType original_type) const {

  if (original_type == Song::FileType_Stream) return Song::FileType_Unknown;

  const MusicStorage::TranscodeMode mode = destination_->GetTranscodeMode();
  const Song::FileType format = destination_->GetTranscodeFormat();

  switch (mode) {
    case MusicStorage::Transcode_Never:
      return Song::FileType_Unknown;

    case MusicStorage::Transcode_Always:
      if (original_type == format) return Song::FileType_Unknown;
      return format;

    case MusicStorage::Transcode_Unsupported:
      if (supported_filetypes_.isEmpty() || supported_filetypes_.contains(original_type)) return Song::FileType_Unknown;

      if (format != Song::FileType_Unknown) return format;

      // The user hasn't visited the device properties page yet to set a preferred format for the device, so we have to pick the best available one.
      return Transcoder::PickBestFormat(supported_filetypes_);
  }
  return Song::FileType_Unknown;

}
#endif

void Organise::SetSongProgress(float progress, bool transcoded) {

  const int max = transcoded ? 50 : 100;
  current_copy_progress_ = (transcoded ? 50 : 0) + qBound(0, static_cast<int>(progress * max), max - 1);
  UpdateProgress();

}

void Organise::UpdateProgress() {

  const int total = task_count_ * 100;

#ifdef HAVE_GSTREAMER
  // Update transcoding progress
  QMap<QString, float> transcode_progress = transcoder_->GetProgress();
  for (const QString &filename : transcode_progress.keys()) {
    if (!tasks_transcoding_.contains(filename)) continue;
    tasks_transcoding_[filename].transcode_progress_ = transcode_progress[filename];
  }
#endif

  // Count the progress of all tasks that are in the queue.
  // Files that need transcoding total 50 for the transcode and 50 for the copy, files that only need to be copied total 100.
  int progress = tasks_complete_ * 100;

  for (const Task &task : tasks_pending_) {
    progress += qBound(0, static_cast<int>(task.transcode_progress_ * 50), 50);
  }
#ifdef HAVE_GSTREAMER
  for (const Task &task : tasks_transcoding_.values()) {
    progress += qBound(0, static_cast<int>(task.transcode_progress_ * 50), 50);
  }
#endif

  // Add the progress of the track that's currently copying
  progress += current_copy_progress_;

  task_manager_->SetTaskProgress(task_id_, progress, total);

}

#ifdef HAVE_GSTREAMER
void Organise::FileTranscoded(const QString &input, const QString &output, bool success) {

  qLog(Info) << "File finished" << input << success;
  transcode_progress_timer_.stop();

  Task task = tasks_transcoding_.take(input);
  if (!success) {
    files_with_errors_ << input;
  }
  else {
    tasks_pending_ << task;
  }
  QTimer::singleShot(0, this, SLOT(ProcessSomeFiles()));

}
#endif

void Organise::timerEvent(QTimerEvent *e) {

  QObject::timerEvent(e);

#ifdef HAVE_GSTREAMER
  if (e->timerId() == transcode_progress_timer_.timerId()) {
    UpdateProgress();
  }
#endif

}

