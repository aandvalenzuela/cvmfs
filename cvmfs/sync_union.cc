/**
 * This file is part of the CernVM File System
 */

#define __STDC_FORMAT_MACROS

#include "sync_union.h"
#include "sync_mediator.h"

using namespace std;  // NOLINT

namespace publish {

SyncUnion::SyncUnion(SyncMediator *mediator, const std::string &rdonly_path,
                     const std::string &union_path,
                     const std::string &scratch_path)
    : rdonly_path_(rdonly_path),
      scratch_path_(scratch_path),
      union_path_(union_path),
      mediator_(mediator),
      initialized_(false) {}

bool SyncUnion::Initialize() {
  mediator_->RegisterUnionEngine(this);
  initialized_ = true;
  return true;
}

SyncItem SyncUnion::CreateSyncItem(const std::string &relative_parent_path,
                                   const std::string &filename,
                                   const SyncItemType entry_type) const {
  SyncItem entry(relative_parent_path, filename, this, entry_type);
  PreprocessSyncItem(&entry);
  if (entry_type == kItemFile) {
    entry.SetExternalData(mediator_->IsExternalData());
    entry.SetCompressionAlgorithm(mediator_->GetCompressionAlgorithm());
  }
  return entry;
}

void SyncUnion::PreprocessSyncItem(SyncItem *entry) const {
  if (IsWhiteoutEntry(*entry)) {
    entry->MarkAsWhiteout(UnwindWhiteoutFilename(*entry));
  }

  if (entry->IsDirectory() && IsOpaqueDirectory(*entry)) {
    entry->MarkAsOpaqueDirectory();
  }
}

bool SyncUnion::IgnoreFilePredicate(const std::string &parent_dir,
                                    const std::string &filename) {
  return false;
}

bool SyncUnion::ProcessDirectory(const string &parent_dir,
                                 const string &dir_name) {
  LogCvmfs(kLogUnionFs, kLogDebug, "SyncUnion::ProcessDirectory(%s, %s)",
           parent_dir.c_str(), dir_name.c_str());
  SyncItem entry = CreateSyncItem(parent_dir, dir_name, kItemDir);

  if (entry.IsNew()) {
    mediator_->Add(entry);
    // Recursion stops here. All content of new directory
    // is added later by the SyncMediator
    return false;
  } else {                            // directory already existed...
    if (entry.IsOpaqueDirectory()) {  // was directory completely overwritten?
      mediator_->Replace(entry);
      return false;  // <-- replace does not need any further recursion
    } else {  // directory was just changed internally... only touch needed
      mediator_->Touch(entry);
      return true;
    }
  }
}

void SyncUnion::ProcessRegularFile(const string &parent_dir,
                                   const string &filename) {
  LogCvmfs(kLogUnionFs, kLogDebug, "SyncUnion::ProcessRegularFile(%s, %s)",
           parent_dir.c_str(), filename.c_str());
  SyncItem entry = CreateSyncItem(parent_dir, filename, kItemFile);
  ProcessFile(entry);
}

void SyncUnion::ProcessSymlink(const string &parent_dir,
                               const string &link_name) {
  LogCvmfs(kLogUnionFs, kLogDebug, "SyncUnion::ProcessSymlink(%s, %s)",
           parent_dir.c_str(), link_name.c_str());
  SyncItem entry = CreateSyncItem(parent_dir, link_name, kItemSymlink);
  ProcessFile(entry);
}

void SyncUnion::ProcessFile(const SyncItem &entry) {
  LogCvmfs(kLogUnionFs, kLogDebug, "SyncUnion::ProcessFile(%s)",
           entry.filename().c_str());
  if (entry.IsWhiteout()) {
    mediator_->Remove(entry);
  } else {
    if (entry.IsNew()) {
      LogCvmfs(kLogUnionFs, kLogVerboseMsg, "processing file [%s] as new (add)",
               entry.filename().c_str());
      mediator_->Add(entry);
    } else {
      LogCvmfs(kLogUnionFs, kLogVerboseMsg,
               "processing file [%s] as existing (touch)",
               entry.filename().c_str());
      mediator_->Touch(entry);
    }
  }
}

void SyncUnion::EnterDirectory(const string &parent_dir,
                               const string &dir_name) {
  SyncItem entry = CreateSyncItem(parent_dir, dir_name, kItemDir);
  mediator_->EnterDirectory(entry);
}

void SyncUnion::LeaveDirectory(const string &parent_dir,
                               const string &dir_name) {
  SyncItem entry = CreateSyncItem(parent_dir, dir_name, kItemDir);
  mediator_->LeaveDirectory(entry);
}

void SyncUnion::ProcessCharacterDevice(const std::string &parent_dir,
                                       const std::string &filename) {
  LogCvmfs(kLogUnionFs, kLogDebug,
           "SyncUnionOverlayfs::ProcessCharacterDevice(%s, %s)",
           parent_dir.c_str(), filename.c_str());
  SyncItem entry = CreateSyncItem(parent_dir, filename, kItemCharacterDevice);
  ProcessFile(entry);
}

void SyncUnion::ProcessBlockDevice(const std::string &parent_dir,
                                   const std::string &filename) {
  LogCvmfs(kLogUnionFs, kLogDebug,
           "SyncUnionOverlayfs::ProcessBlockDevice(%s, %s)", parent_dir.c_str(),
           filename.c_str());
  SyncItem entry = CreateSyncItem(parent_dir, filename, kItemBlockDevice);
  ProcessFile(entry);
}

void SyncUnion::ProcessFifo(const std::string &parent_dir,
                            const std::string &filename) {
  LogCvmfs(kLogUnionFs, kLogDebug, "SyncUnionOverlayfs::ProcessFifo(%s, %s)",
           parent_dir.c_str(), filename.c_str());
  SyncItem entry = CreateSyncItem(parent_dir, filename, kItemFifo);
  ProcessFile(entry);
}

void SyncUnion::ProcessSocket(const std::string &parent_dir,
                              const std::string &filename) {
  LogCvmfs(kLogUnionFs, kLogDebug, "SyncUnionOverlayfs::ProcessSocket(%s, %s)",
           parent_dir.c_str(), filename.c_str());
  SyncItem entry = CreateSyncItem(parent_dir, filename, kItemSocket);
  ProcessFile(entry);
}

}  // namespace publish
