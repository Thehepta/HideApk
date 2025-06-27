/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/mapped_file.h>
#include <android-base/memory.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <gtest/gtest.h>
#include <ziparchive/zip_archive.h>
#include <ziparchive/zip_archive_stream_entry.h>

#include "zip_archive_common.h"
#include "zip_archive_private.h"

static std::string test_data_dir = android::base::GetExecutableDirectory() + "/testdata";

static const std::string kValidZip = "valid.zip";
static const std::string kLargeZip = "large.zip";
static const std::string kBadCrcZip = "bad_crc.zip";

static const std::vector<uint8_t> kATxtContents{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'a',
                                                'b', 'c', 'd', 'e', 'f', 'g', 'h', '\n'};

static const std::vector<uint8_t> kATxtContentsCompressed{'K', 'L', 'J', 'N', 'I',  'M', 'K',
                                                          207, 'H', 132, 210, '\\', '\0'};

static const std::vector<uint8_t> kBTxtContents{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', '\n'};

static int32_t OpenArchiveWrapper(const std::string& name, ZipArchiveHandle* handle) {
  const std::string abs_path = test_data_dir + "/" + name;
  return OpenArchive(abs_path.c_str(), handle);
}

class CdEntryMapTest : public ::testing::Test {
 protected:
  void SetUp() override {
    names_ = {
        "a.txt", "b.txt", "b/", "b/c.txt", "b/d.txt",
    };
    separator_ = "separator";
    header_ = "metadata";
    joined_names_ = header_ + android::base::Join(names_, separator_);
    base_ptr_ = reinterpret_cast<uint8_t*>(&joined_names_[0]);

    uint16_t num_entries = static_cast<uint16_t>(names_.size());
    entry_maps_.emplace_back(new CdEntryMapZip32<ZipStringOffset20>(num_entries));
    entry_maps_.emplace_back(new CdEntryMapZip32<ZipStringOffset32>(num_entries));
    entry_maps_.emplace_back(new CdEntryMapZip64());
    for (auto& cd_map : entry_maps_) {
      ASSERT_NE(nullptr, cd_map);
      size_t offset = header_.size();
      for (const auto& name : names_) {
        auto status = cd_map->AddToMap(
            std::string_view{joined_names_.c_str() + offset, name.size()}, base_ptr_);
        ASSERT_EQ(0, status);
        offset += name.size() + separator_.size();
      }
    }
  }

  std::vector<std::string> names_;
  // A continuous region of memory serves as a mock of the central directory.
  std::string joined_names_;
  // We expect some metadata at the beginning of the central directory and between filenames.
  std::string header_;
  std::string separator_;

  std::vector<std::unique_ptr<CdEntryMapInterface>> entry_maps_;
  uint8_t* base_ptr_{nullptr};  // Points to the start of the central directory.
};

TEST_F(CdEntryMapTest, AddDuplicatedEntry) {
  for (auto& cd_map : entry_maps_) {
    std::string_view name = "b.txt";
    ASSERT_NE(0, cd_map->AddToMap(name, base_ptr_));
  }
}

TEST_F(CdEntryMapTest, FindEntry) {
  for (auto& cd_map : entry_maps_) {
    uint64_t expected_offset = header_.size();
    for (const auto& name : names_) {
      auto [status, offset] = cd_map->GetCdEntryOffset(name, base_ptr_);
      ASSERT_EQ(status, kSuccess);
      ASSERT_EQ(offset, expected_offset);
      expected_offset += name.size() + separator_.size();
    }
  }
}

TEST_F(CdEntryMapTest, Iteration) {
  std::set<std::string_view> expected(names_.begin(), names_.end());
  for (auto& cd_map : entry_maps_) {
    cd_map->ResetIteration();
    std::set<std::string_view> entry_set;
    auto ret = cd_map->Next(base_ptr_);
    while (ret != std::pair<std::string_view, uint64_t>{}) {
      auto [it, insert_status] = entry_set.insert(ret.first);
      ASSERT_TRUE(insert_status);
      ret = cd_map->Next(base_ptr_);
    }
    ASSERT_EQ(expected, entry_set);
  }
}

TEST(ziparchive, Open) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));
  const auto& mappedFile = handle->mapped_zip;
  if constexpr (sizeof(void*) < 8) {
    ASSERT_EQ(nullptr, mappedFile.GetBasePtr());
  } else {
    ASSERT_NE(nullptr, mappedFile.GetBasePtr());
  }
  CloseArchive(handle);

  ASSERT_EQ(kInvalidEntryName, OpenArchiveWrapper("bad_filename.zip", &handle));
  CloseArchive(handle);
}

TEST(ziparchive, OutOfBound) {
  ZipArchiveHandle handle;
  ASSERT_EQ(kInvalidOffset, OpenArchiveWrapper("crash.apk", &handle));
  CloseArchive(handle);
}

TEST(ziparchive, EmptyArchive) {
  ZipArchiveHandle handle;
  ASSERT_EQ(kEmptyArchive, OpenArchiveWrapper("empty.zip", &handle));
  CloseArchive(handle);
}

TEST(ziparchive, ZeroSizeCentralDirectory) {
  ZipArchiveHandle handle;
  ASSERT_EQ(kInvalidFile, OpenArchiveWrapper("zero-size-cd.zip", &handle));
  CloseArchive(handle);
}

TEST(ziparchive, OpenMissing) {
  ZipArchiveHandle handle;
  ASSERT_NE(0, OpenArchiveWrapper("missing.zip", &handle));

  // Confirm the file descriptor is not going to be mistaken for a valid one.
  ASSERT_EQ(-1, GetFileDescriptor(handle));
}

TEST(ziparchive, OpenAssumeFdOwnership) {
  int fd = open((test_data_dir + "/" + kValidZip).c_str(), O_RDONLY | O_BINARY);
  ASSERT_NE(-1, fd);
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFd(fd, "OpenWithAssumeFdOwnership", &handle));
  CloseArchive(handle);
  ASSERT_EQ(-1, lseek(fd, 0, SEEK_SET));
  ASSERT_EQ(EBADF, errno);
}

TEST(ziparchive, OpenDoNotAssumeFdOwnership) {
  int fd = open((test_data_dir + "/" + kValidZip).c_str(), O_RDONLY | O_BINARY);
  ASSERT_NE(-1, fd);
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFd(fd, "OpenWithAssumeFdOwnership", &handle, false));
  CloseArchive(handle);
  ASSERT_EQ(0, lseek(fd, 0, SEEK_SET));
  close(fd);
}

TEST(ziparchive, OpenAssumeFdRangeOwnership) {
  int fd = open((test_data_dir + "/" + kValidZip).c_str(), O_RDONLY | O_BINARY);
  ASSERT_NE(-1, fd);
  const off64_t length = lseek64(fd, 0, SEEK_END);
  ASSERT_NE(-1, length);
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFdRange(fd, "OpenWithAssumeFdOwnership", &handle,
                                  static_cast<size_t>(length), 0));
  CloseArchive(handle);
  ASSERT_EQ(-1, lseek(fd, 0, SEEK_SET));
  ASSERT_EQ(EBADF, errno);
}

TEST(ziparchive, OpenDoNotAssumeFdRangeOwnership) {
  int fd = open((test_data_dir + "/" + kValidZip).c_str(), O_RDONLY | O_BINARY);
  ASSERT_NE(-1, fd);
  const off64_t length = lseek(fd, 0, SEEK_END);
  ASSERT_NE(-1, length);
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFdRange(fd, "OpenWithAssumeFdOwnership", &handle,
                                  static_cast<size_t>(length), 0, false));
  CloseArchive(handle);
  ASSERT_EQ(0, lseek(fd, 0, SEEK_SET));
  close(fd);
}

TEST(ziparchive, Iteration_std_string_view) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  void* iteration_cookie;
  ASSERT_EQ(0, StartIteration(handle, &iteration_cookie));

  ZipEntry64 data;
  std::vector<std::string_view> names;
  std::string_view name;
  while (Next(iteration_cookie, &data, &name) == 0) names.push_back(name);

  // Assert that the names are as expected.
  std::vector<std::string_view> expected_names{"a.txt", "b.txt", "b/", "b/c.txt", "b/d.txt"};
  std::sort(names.begin(), names.end());
  ASSERT_EQ(expected_names, names);

  CloseArchive(handle);
}

static void AssertIterationNames(void* iteration_cookie,
                                 const std::vector<std::string>& expected_names_sorted) {
  ZipEntry64 data;
  std::vector<std::string> names;
  std::string_view name;
  for (size_t i = 0; i < expected_names_sorted.size(); ++i) {
    ASSERT_EQ(0, Next(iteration_cookie, &data, &name));
    names.push_back(std::string(name));
  }
  // End of iteration.
  ASSERT_EQ(-1, Next(iteration_cookie, &data, &name));
  // Assert that the names are as expected.
  std::sort(names.begin(), names.end());
  ASSERT_EQ(expected_names_sorted, names);
}

static void AssertIterationOrder(const std::string_view prefix, const std::string_view suffix,
                                 const std::vector<std::string>& expected_names_sorted) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  void* iteration_cookie;
  ASSERT_EQ(0, StartIteration(handle, &iteration_cookie, prefix, suffix));
  AssertIterationNames(iteration_cookie, expected_names_sorted);
  CloseArchive(handle);
}

static void AssertIterationOrderWithMatcher(std::function<bool(std::string_view)> matcher,
                                            const std::vector<std::string>& expected_names_sorted) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  void* iteration_cookie;
  ASSERT_EQ(0, StartIteration(handle, &iteration_cookie, matcher));
  AssertIterationNames(iteration_cookie, expected_names_sorted);
  CloseArchive(handle);
}

TEST(ziparchive, Iteration) {
  static const std::vector<std::string> kExpectedMatchesSorted = {"a.txt", "b.txt", "b/", "b/c.txt",
                                                                  "b/d.txt"};

  AssertIterationOrder("", "", kExpectedMatchesSorted);
}

TEST(ziparchive, IterationWithPrefix) {
  static const std::vector<std::string> kExpectedMatchesSorted = {"b/", "b/c.txt", "b/d.txt"};

  AssertIterationOrder("b/", "", kExpectedMatchesSorted);
}

TEST(ziparchive, IterationWithSuffix) {
  static const std::vector<std::string> kExpectedMatchesSorted = {"a.txt", "b.txt", "b/c.txt",
                                                                  "b/d.txt"};

  AssertIterationOrder("", ".txt", kExpectedMatchesSorted);
}

TEST(ziparchive, IterationWithPrefixAndSuffix) {
  static const std::vector<std::string> kExpectedMatchesSorted = {"b.txt", "b/c.txt", "b/d.txt"};

  AssertIterationOrder("b", ".txt", kExpectedMatchesSorted);
}

TEST(ziparchive, IterationWithAdditionalMatchesExactly) {
  static const std::vector<std::string> kExpectedMatchesSorted = {"a.txt"};
  auto matcher = [](std::string_view name) { return name == "a.txt"; };
  AssertIterationOrderWithMatcher(matcher, kExpectedMatchesSorted);
}

TEST(ziparchive, IterationWithAdditionalMatchesWithSuffix) {
  static const std::vector<std::string> kExpectedMatchesSorted = {"a.txt", "b.txt", "b/c.txt",
                                                                  "b/d.txt"};
  auto matcher = [](std::string_view name) {
    return name == "a.txt" || android::base::EndsWith(name, ".txt");
  };
  AssertIterationOrderWithMatcher(matcher, kExpectedMatchesSorted);
}

TEST(ziparchive, IterationWithAdditionalMatchesWithPrefixAndSuffix) {
  static const std::vector<std::string> kExpectedMatchesSorted = {"a.txt", "b/c.txt", "b/d.txt"};
  auto matcher = [](std::string_view name) {
    return name == "a.txt" ||
           (android::base::EndsWith(name, ".txt") && android::base::StartsWith(name, "b/"));
  };
  AssertIterationOrderWithMatcher(matcher, kExpectedMatchesSorted);
}

TEST(ziparchive, IterationWithBadPrefixAndSuffix) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  void* iteration_cookie;
  ASSERT_EQ(0, StartIteration(handle, &iteration_cookie, "x", "y"));

  ZipEntry64 data;
  std::string_view name;

  // End of iteration.
  ASSERT_EQ(-1, Next(iteration_cookie, &data, &name));

  CloseArchive(handle);
}

TEST(ziparchive, FindEntry) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  ZipEntry64 data;
  ASSERT_EQ(0, FindEntry(handle, "a.txt", &data));

  // Known facts about a.txt, from zipinfo -v.
  ASSERT_EQ(63, data.offset);
  ASSERT_EQ(kCompressDeflated, data.method);
  ASSERT_EQ(17u, data.uncompressed_length);
  ASSERT_EQ(13u, data.compressed_length);
  ASSERT_EQ(0x950821c5, data.crc32);
  ASSERT_EQ(static_cast<uint32_t>(0x438a8005), data.mod_time);

  // An entry that doesn't exist. Should be a negative return code.
  ASSERT_LT(FindEntry(handle, "this file does not exist", &data), 0);

  CloseArchive(handle);
}

TEST(ziparchive, FindEntry_empty) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  ZipEntry64 data;
  ASSERT_EQ(kInvalidEntryName, FindEntry(handle, "", &data));

  CloseArchive(handle);
}

TEST(ziparchive, FindEntry_too_long) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  std::string very_long_name(65536, 'x');
  ZipEntry64 data;
  ASSERT_EQ(kInvalidEntryName, FindEntry(handle, very_long_name, &data));

  CloseArchive(handle);
}

TEST(ziparchive, TestInvalidDeclaredLength) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper("declaredlength.zip", &handle));

  void* iteration_cookie;
  ASSERT_EQ(0, StartIteration(handle, &iteration_cookie));

  std::string_view name;
  ZipEntry64 data;

  ASSERT_EQ(Next(iteration_cookie, &data, &name), 0);
  ASSERT_EQ(Next(iteration_cookie, &data, &name), 0);

  CloseArchive(handle);
}

TEST(ziparchive, OpenArchiveFdRange) {
  TemporaryFile tmp_file;
  ASSERT_NE(-1, tmp_file.fd);

  const std::string leading_garbage(21, 'x');
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, leading_garbage.c_str(),
                                        leading_garbage.size()));

  std::string valid_content;
  ASSERT_TRUE(android::base::ReadFileToString(test_data_dir + "/" + kValidZip, &valid_content));
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, valid_content.c_str(), valid_content.size()));

  const std::string ending_garbage(42, 'x');
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, ending_garbage.c_str(),
                                        ending_garbage.size()));

  ZipArchiveHandle handle;
  ASSERT_EQ(0, lseek(tmp_file.fd, 0, SEEK_SET));
  ASSERT_EQ(0, OpenArchiveFdRange(tmp_file.fd, "OpenArchiveFdRange", &handle,
                                  valid_content.size(),
                                  static_cast<off64_t>(leading_garbage.size())));

  // An entry that's deflated.
  ZipEntry64 data;
  ASSERT_EQ(0, FindEntry(handle, "a.txt", &data));
  const auto a_size = static_cast<size_t>(data.uncompressed_length);
  ASSERT_EQ(a_size, kATxtContents.size());
  auto buffer = std::unique_ptr<uint8_t[]>(new uint8_t[a_size]);
  ASSERT_EQ(0, ExtractToMemory(handle, &data, buffer.get(), a_size));
  ASSERT_EQ(0, memcmp(buffer.get(), kATxtContents.data(), a_size));

  // An entry that's stored.
  ASSERT_EQ(0, FindEntry(handle, "b.txt", &data));
  const auto b_size = static_cast<size_t>(data.uncompressed_length);
  ASSERT_EQ(b_size, kBTxtContents.size());
  buffer = std::unique_ptr<uint8_t[]>(new uint8_t[b_size]);
  ASSERT_EQ(0, ExtractToMemory(handle, &data, buffer.get(), b_size));
  ASSERT_EQ(0, memcmp(buffer.get(), kBTxtContents.data(), b_size));

  CloseArchive(handle);
}

TEST(ziparchive, ExtractToMemory) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  // An entry that's deflated.
  ZipEntry64 data;
  ASSERT_EQ(0, FindEntry(handle, "a.txt", &data));
  const auto a_size = static_cast<size_t>(data.uncompressed_length);
  ASSERT_EQ(a_size, kATxtContents.size());
  uint8_t* buffer = new uint8_t[a_size];
  ASSERT_EQ(0, ExtractToMemory(handle, &data, buffer, a_size));
  ASSERT_EQ(0, memcmp(buffer, kATxtContents.data(), a_size));
  delete[] buffer;

  // An entry that's stored.
  ASSERT_EQ(0, FindEntry(handle, "b.txt", &data));
  const auto b_size = static_cast<size_t>(data.uncompressed_length);
  ASSERT_EQ(b_size, kBTxtContents.size());
  buffer = new uint8_t[b_size];
  ASSERT_EQ(0, ExtractToMemory(handle, &data, buffer, b_size));
  ASSERT_EQ(0, memcmp(buffer, kBTxtContents.data(), b_size));
  delete[] buffer;

  CloseArchive(handle);
}

static const uint32_t kEmptyEntriesZip[] = {
    0x04034b50, 0x0000000a, 0x63600000, 0x00004438, 0x00000000, 0x00000000, 0x00090000,
    0x6d65001c, 0x2e797470, 0x55747874, 0x03000954, 0x52e25c13, 0x52e25c24, 0x000b7875,
    0x42890401, 0x88040000, 0x50000013, 0x1e02014b, 0x00000a03, 0x60000000, 0x00443863,
    0x00000000, 0x00000000, 0x09000000, 0x00001800, 0x00000000, 0xa0000000, 0x00000081,
    0x706d6500, 0x742e7974, 0x54557478, 0x13030005, 0x7552e25c, 0x01000b78, 0x00428904,
    0x13880400, 0x4b500000, 0x00000605, 0x00010000, 0x004f0001, 0x00430000, 0x00000000};

// This is a zip file containing a single entry (ab.txt) that contains
// 90072 repetitions of the string "ab\n" and has an uncompressed length
// of 270216 bytes.
static const uint16_t kAbZip[] = {
    0x4b50, 0x0403, 0x0014, 0x0000, 0x0008, 0x51d2, 0x4698, 0xc4b0, 0x2cda, 0x011b, 0x0000, 0x1f88,
    0x0004, 0x0006, 0x001c, 0x6261, 0x742e, 0x7478, 0x5455, 0x0009, 0x7c03, 0x3a09, 0x7c55, 0x3a09,
    0x7555, 0x0b78, 0x0100, 0x8904, 0x0042, 0x0400, 0x1388, 0x0000, 0xc2ed, 0x0d31, 0x0000, 0x030c,
    0x7fa0, 0x3b2e, 0x22ff, 0xa2aa, 0x841f, 0x45fc, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555, 0x5555,
    0x5555, 0x5555, 0x5555, 0x5555, 0xdd55, 0x502c, 0x014b, 0x1e02, 0x1403, 0x0000, 0x0800, 0xd200,
    0x9851, 0xb046, 0xdac4, 0x1b2c, 0x0001, 0x8800, 0x041f, 0x0600, 0x1800, 0x0000, 0x0000, 0x0100,
    0x0000, 0xa000, 0x0081, 0x0000, 0x6100, 0x2e62, 0x7874, 0x5574, 0x0554, 0x0300, 0x097c, 0x553a,
    0x7875, 0x000b, 0x0401, 0x4289, 0x0000, 0x8804, 0x0013, 0x5000, 0x054b, 0x0006, 0x0000, 0x0100,
    0x0100, 0x4c00, 0x0000, 0x5b00, 0x0001, 0x0000, 0x0000};

static const std::string kAbTxtName("ab.txt");
static const size_t kAbUncompressedSize = 270216;

TEST(ziparchive, EmptyEntries) {
  TemporaryFile tmp_file;
  ASSERT_NE(-1, tmp_file.fd);
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, kEmptyEntriesZip, sizeof(kEmptyEntriesZip)));

  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFd(tmp_file.fd, "EmptyEntriesTest", &handle, false));

  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, "empty.txt", &entry));
  ASSERT_EQ(0u, entry.uncompressed_length);
  // Extraction to a 1 byte buffer should succeed.
  uint8_t buffer[1];
  ASSERT_EQ(0, ExtractToMemory(handle, &entry, buffer, 1));
  // Extraction to an empty buffer should succeed.
  ASSERT_EQ(0, ExtractToMemory(handle, &entry, nullptr, 0));

  TemporaryFile tmp_output_file;
  ASSERT_NE(-1, tmp_output_file.fd);
  ASSERT_EQ(0, ExtractEntryToFile(handle, &entry, tmp_output_file.fd));

  struct stat stat_buf;
  ASSERT_EQ(0, fstat(tmp_output_file.fd, &stat_buf));
  ASSERT_EQ(0, stat_buf.st_size);
}

TEST(ziparchive, EntryLargerThan32K) {
  TemporaryFile tmp_file;
  ASSERT_NE(-1, tmp_file.fd);
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, reinterpret_cast<const uint8_t*>(kAbZip),
                                        sizeof(kAbZip) - 1));
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFd(tmp_file.fd, "EntryLargerThan32KTest", &handle, false));

  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, kAbTxtName, &entry));
  ASSERT_EQ(kAbUncompressedSize, entry.uncompressed_length);

  // Extract the entry to memory.
  std::vector<uint8_t> buffer(kAbUncompressedSize);
  ASSERT_EQ(0, ExtractToMemory(handle, &entry, &buffer[0], static_cast<uint32_t>(buffer.size())));

  // Extract the entry to a file.
  TemporaryFile tmp_output_file;
  ASSERT_NE(-1, tmp_output_file.fd);
  ASSERT_EQ(0, ExtractEntryToFile(handle, &entry, tmp_output_file.fd));

  // Make sure the extracted file size is as expected.
  struct stat stat_buf;
  ASSERT_EQ(0, fstat(tmp_output_file.fd, &stat_buf));
  ASSERT_EQ(kAbUncompressedSize, static_cast<size_t>(stat_buf.st_size));

  // Read the file back to a buffer and make sure the contents are
  // the same as the memory buffer we extracted directly to.
  std::vector<uint8_t> file_contents(kAbUncompressedSize);
  ASSERT_EQ(0, lseek(tmp_output_file.fd, 0, SEEK_SET));
  ASSERT_TRUE(android::base::ReadFully(tmp_output_file.fd, &file_contents[0], file_contents.size()));
  ASSERT_EQ(file_contents, buffer);

  for (int i = 0; i < 90072; ++i) {
    const uint8_t* line = &file_contents[0] + (3 * i);
    ASSERT_EQ('a', line[0]);
    ASSERT_EQ('b', line[1]);
    ASSERT_EQ('\n', line[2]);
  }
}

TEST(ziparchive, TrailerAfterEOCD) {
  TemporaryFile tmp_file;
  ASSERT_NE(-1, tmp_file.fd);

  // Create a file with 8 bytes of random garbage.
  static const uint8_t trailer[] = {'A', 'n', 'd', 'r', 'o', 'i', 'd', 'z'};
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, kEmptyEntriesZip, sizeof(kEmptyEntriesZip)));
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, trailer, sizeof(trailer)));

  ZipArchiveHandle handle;
  ASSERT_GT(0, OpenArchiveFd(tmp_file.fd, "EmptyEntriesTest", &handle, false));
}

TEST(ziparchive, ExtractToFile) {
  TemporaryFile tmp_file;
  ASSERT_NE(-1, tmp_file.fd);
  const uint8_t data[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
  const size_t data_size = sizeof(data);

  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, data, data_size));

  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kValidZip, &handle));

  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, "a.txt", &entry));
  ASSERT_EQ(0, ExtractEntryToFile(handle, &entry, tmp_file.fd));

  // Assert that the first 8 bytes of the file haven't been clobbered.
  uint8_t read_buffer[data_size];
  ASSERT_EQ(0, lseek(tmp_file.fd, 0, SEEK_SET));
  ASSERT_TRUE(android::base::ReadFully(tmp_file.fd, read_buffer, data_size));
  ASSERT_EQ(0, memcmp(read_buffer, data, data_size));

  // Assert that the remainder of the file contains the incompressed data.
  std::vector<uint8_t> uncompressed_data(static_cast<size_t>(entry.uncompressed_length));
  ASSERT_TRUE(android::base::ReadFully(tmp_file.fd, uncompressed_data.data(),
                                       static_cast<size_t>(entry.uncompressed_length)));
  ASSERT_EQ(0, memcmp(&uncompressed_data[0], kATxtContents.data(), kATxtContents.size()));

  // Assert that the total length of the file is sane
  ASSERT_EQ(static_cast<ssize_t>(data_size + kATxtContents.size()),
            lseek(tmp_file.fd, 0, SEEK_END));
}

#if !defined(_WIN32)
TEST(ziparchive, OpenFromMemory) {
  const std::string zip_path = test_data_dir + "/dummy-update.zip";
  android::base::unique_fd fd(open(zip_path.c_str(), O_RDONLY | O_BINARY));
  ASSERT_NE(-1, fd);
  struct stat sb;
  ASSERT_EQ(0, fstat(fd, &sb));

  // Memory map the file first and open the archive from the memory region.
  auto file_map{
      android::base::MappedFile::FromFd(fd, 0, static_cast<size_t>(sb.st_size), PROT_READ)};
  ZipArchiveHandle handle;
  ASSERT_EQ(0,
            OpenArchiveFromMemory(file_map->data(), file_map->size(), zip_path.c_str(), &handle));

  // Assert one entry can be found and extracted correctly.
  ZipEntry64 binary_entry;
  ASSERT_EQ(0, FindEntry(handle, "META-INF/com/google/android/update-binary", &binary_entry));
  TemporaryFile tmp_binary;
  ASSERT_NE(-1, tmp_binary.fd);
  ASSERT_EQ(0, ExtractEntryToFile(handle, &binary_entry, tmp_binary.fd));
}
#endif

static void ZipArchiveStreamTest(ZipArchiveHandle& handle, const std::string& entry_name, bool raw,
                                 bool verified, ZipEntry* entry, std::vector<uint8_t>* read_data) {
  ASSERT_EQ(0, FindEntry(handle, entry_name, entry));
  std::unique_ptr<ZipArchiveStreamEntry> stream;
  if (raw) {
    stream.reset(ZipArchiveStreamEntry::CreateRaw(handle, *entry));
    if (entry->method == kCompressStored) {
      read_data->resize(static_cast<size_t>(entry->uncompressed_length));
    } else {
      read_data->resize(static_cast<size_t>(entry->compressed_length));
    }
  } else {
    stream.reset(ZipArchiveStreamEntry::Create(handle, *entry));
    read_data->resize(static_cast<size_t>(entry->uncompressed_length));
  }
  uint8_t* read_data_ptr = read_data->data();
  ASSERT_TRUE(stream.get() != nullptr);
  const std::vector<uint8_t>* data;
  uint64_t total_size = 0;
  while ((data = stream->Read()) != nullptr) {
    total_size += data->size();
    memcpy(read_data_ptr, data->data(), data->size());
    read_data_ptr += data->size();
  }
  ASSERT_EQ(verified, stream->Verify());
  ASSERT_EQ(total_size, read_data->size());
}

static void ZipArchiveStreamTestUsingContents(const std::string& zip_file,
                                              const std::string& entry_name,
                                              const std::vector<uint8_t>& contents, bool raw) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(zip_file, &handle));

  ZipEntry entry;
  std::vector<uint8_t> read_data;
  ZipArchiveStreamTest(handle, entry_name, raw, true, &entry, &read_data);

  ASSERT_EQ(contents.size(), read_data.size());
  ASSERT_TRUE(memcmp(read_data.data(), contents.data(), read_data.size()) == 0);

  CloseArchive(handle);
}

static void ZipArchiveStreamTestUsingMemory(const std::string& zip_file,
                                            const std::string& entry_name) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(zip_file, &handle));

  ZipEntry entry;
  std::vector<uint8_t> read_data;
  ZipArchiveStreamTest(handle, entry_name, false, true, &entry, &read_data);

  std::vector<uint8_t> cmp_data(static_cast<size_t>(entry.uncompressed_length));
  ASSERT_EQ(entry.uncompressed_length, read_data.size());
  ASSERT_EQ(
      0, ExtractToMemory(handle, &entry, cmp_data.data(), static_cast<uint32_t>(cmp_data.size())));
  ASSERT_TRUE(memcmp(read_data.data(), cmp_data.data(), read_data.size()) == 0);

  CloseArchive(handle);
}

TEST(ziparchive, StreamCompressed) {
  ZipArchiveStreamTestUsingContents(kValidZip, "a.txt", kATxtContents, false);
}

TEST(ziparchive, StreamUncompressed) {
  ZipArchiveStreamTestUsingContents(kValidZip, "b.txt", kBTxtContents, false);
}

TEST(ziparchive, StreamRawCompressed) {
  ZipArchiveStreamTestUsingContents(kValidZip, "a.txt", kATxtContentsCompressed, true);
}

TEST(ziparchive, StreamRawUncompressed) {
  ZipArchiveStreamTestUsingContents(kValidZip, "b.txt", kBTxtContents, true);
}

TEST(ziparchive, StreamLargeCompressed) {
  ZipArchiveStreamTestUsingMemory(kLargeZip, "compress.txt");
}

TEST(ziparchive, StreamLargeUncompressed) {
  ZipArchiveStreamTestUsingMemory(kLargeZip, "uncompress.txt");
}

TEST(ziparchive, StreamCompressedBadCrc) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kBadCrcZip, &handle));

  ZipEntry entry;
  std::vector<uint8_t> read_data;
  ZipArchiveStreamTest(handle, "a.txt", false, false, &entry, &read_data);

  CloseArchive(handle);
}

TEST(ziparchive, StreamUncompressedBadCrc) {
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveWrapper(kBadCrcZip, &handle));

  ZipEntry entry;
  std::vector<uint8_t> read_data;
  ZipArchiveStreamTest(handle, "b.txt", false, false, &entry, &read_data);

  CloseArchive(handle);
}

// Generated using the following Java program:
//     public static void main(String[] foo) throws Exception {
//       FileOutputStream fos = new
//       FileOutputStream("/tmp/data_descriptor.zip");
//       ZipOutputStream zos = new ZipOutputStream(fos);
//       ZipEntry64 ze = new ZipEntry64("name");
//       ze.setMethod(ZipEntry64.DEFLATED);
//       zos.putNextEntry(ze);
//       zos.write("abdcdefghijk".getBytes());
//       zos.closeEntry();
//       zos.close();
//     }
//
// cat /tmp/data_descriptor.zip | xxd -i
//
static const std::vector<uint8_t> kDataDescriptorZipFile{
    0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x08, 0x08, 0x08, 0x00, 0x30, 0x59, 0xce, 0x4a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6e, 0x61,
    0x6d, 0x65, 0x4b, 0x4c, 0x4a, 0x49, 0x4e, 0x49, 0x4d, 0x4b, 0xcf, 0xc8, 0xcc, 0xca, 0x06, 0x00,
    //[sig---------------], [crc32---------------], [csize---------------], [size----------------]
    0x50, 0x4b, 0x07, 0x08, 0x3d, 0x4e, 0x0e, 0xf9, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x50, 0x4b, 0x01, 0x02, 0x14, 0x00, 0x14, 0x00, 0x08, 0x08, 0x08, 0x00, 0x30, 0x59, 0xce, 0x4a,
    0x3d, 0x4e, 0x0e, 0xf9, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x61,
    0x6d, 0x65, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x32, 0x00,
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};

// The offsets of the data descriptor in this file, so we can mess with
// them later in the test.
static constexpr uint32_t kDataDescriptorOffset = 48;
static constexpr uint32_t kCSizeOffset = kDataDescriptorOffset + 8;
static constexpr uint32_t kSizeOffset = kCSizeOffset + 4;

static void ExtractEntryToMemory(const std::vector<uint8_t>& zip_data,
                                 std::vector<uint8_t>* entry_out, int32_t* error_code_out) {
  TemporaryFile tmp_file;
  ASSERT_NE(-1, tmp_file.fd);
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, &zip_data[0], zip_data.size()));
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFd(tmp_file.fd, "ExtractEntryToMemory", &handle, false));

  // This function expects a variant of kDataDescriptorZipFile, for look for
  // an entry whose name is "name" and whose size is 12 (contents =
  // "abdcdefghijk").
  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, "name", &entry));
  ASSERT_EQ(12u, entry.uncompressed_length);

  entry_out->resize(12);
  (*error_code_out) = ExtractToMemory(handle, &entry, &((*entry_out)[0]), 12);

  CloseArchive(handle);
}

TEST(ziparchive, ValidDataDescriptors) {
  std::vector<uint8_t> entry;
  int32_t error_code = 0;
  ExtractEntryToMemory(kDataDescriptorZipFile, &entry, &error_code);

  ASSERT_EQ(0, error_code);
  ASSERT_EQ(12u, entry.size());
  ASSERT_EQ('a', entry[0]);
  ASSERT_EQ('k', entry[11]);
}

TEST(ziparchive, InvalidDataDescriptors_csize) {
  std::vector<uint8_t> invalid_csize = kDataDescriptorZipFile;
  invalid_csize[kCSizeOffset] = 0xfe;

  std::vector<uint8_t> entry;
  int32_t error_code = 0;
  ExtractEntryToMemory(invalid_csize, &entry, &error_code);

  ASSERT_EQ(kInconsistentInformation, error_code);
}

TEST(ziparchive, InvalidDataDescriptors_size) {
  std::vector<uint8_t> invalid_size = kDataDescriptorZipFile;
  invalid_size[kSizeOffset] = 0xfe;

  std::vector<uint8_t> entry;
  int32_t error_code = 0;
  ExtractEntryToMemory(invalid_size, &entry, &error_code);

  ASSERT_EQ(kInconsistentInformation, error_code);
}

TEST(ziparchive, ErrorCodeString) {
  ASSERT_STREQ("Success", ErrorCodeString(0));

  // Out of bounds.
  ASSERT_STREQ("Unknown return code", ErrorCodeString(1));
  ASSERT_STRNE("Unknown return code", ErrorCodeString(kLastErrorCode));
  ASSERT_STREQ("Unknown return code", ErrorCodeString(kLastErrorCode - 1));

  ASSERT_STREQ("I/O error", ErrorCodeString(kIoError));
}

// A zip file whose local file header at offset zero is corrupted.
//
// ---------------
// cat foo > a.txt
// zip a.zip a.txt
// cat a.zip | xxd -i
//
// Manual changes :
// [2] = 0xff  // Corrupt the LFH signature of entry 0.
// [3] = 0xff  // Corrupt the LFH signature of entry 0.
static const std::vector<uint8_t> kZipFileWithBrokenLfhSignature{
    //[lfh-sig-----------], [lfh contents---------------------------------
    0x50, 0x4b, 0xff, 0xff, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x80,
    //--------------------------------------------------------------------
    0x09, 0x4b, 0xa8, 0x65, 0x32, 0x7e, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00,
    //-------------------------------]  [file-name-----------------], [---
    0x00, 0x00, 0x05, 0x00, 0x1c, 0x00, 0x61, 0x2e, 0x74, 0x78, 0x74, 0x55,
    // entry-contents------------------------------------------------------
    0x54, 0x09, 0x00, 0x03, 0x51, 0x24, 0x8b, 0x59, 0x51, 0x24, 0x8b, 0x59,
    //--------------------------------------------------------------------
    0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0x89, 0x42, 0x00, 0x00, 0x04, 0x88,
    //-------------------------------------], [cd-record-sig-------], [---
    0x13, 0x00, 0x00, 0x66, 0x6f, 0x6f, 0x0a, 0x50, 0x4b, 0x01, 0x02, 0x1e,
    // cd-record-----------------------------------------------------------
    0x03, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x80, 0x09, 0x4b, 0xa8,
    //--------------------------------------------------------------------
    0x65, 0x32, 0x7e, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05,
    //--------------------------------------------------------------------
    0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa0,
    //-]  [lfh-file-header-off-], [file-name-----------------], [extra----
    0x81, 0x00, 0x00, 0x00, 0x00, 0x61, 0x2e, 0x74, 0x78, 0x74, 0x55, 0x54,
    //--------------------------------------------------------------------
    0x05, 0x00, 0x03, 0x51, 0x24, 0x8b, 0x59, 0x75, 0x78, 0x0b, 0x00, 0x01,
    //-------------------------------------------------------], [eocd-sig-
    0x04, 0x89, 0x42, 0x00, 0x00, 0x04, 0x88, 0x13, 0x00, 0x00, 0x50, 0x4b,
    //-------], [---------------------------------------------------------
    0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x4b, 0x00,
    //-------------------------------------------]
    0x00, 0x00, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00};

TEST(ziparchive, BrokenLfhSignature) {
  TemporaryFile tmp_file;
  ASSERT_NE(-1, tmp_file.fd);
  ASSERT_TRUE(android::base::WriteFully(tmp_file.fd, &kZipFileWithBrokenLfhSignature[0],
                                        kZipFileWithBrokenLfhSignature.size()));
  ZipArchiveHandle handle;
  ASSERT_EQ(kInvalidFile, OpenArchiveFd(tmp_file.fd, "LeadingNonZipBytes", &handle, false));
}

class VectorReader final : public zip_archive::Reader {
 public:
  VectorReader(const std::vector<uint8_t>& input) : Reader(), input_(input) {}

  bool ReadAtOffset(uint8_t* buf, size_t len, off64_t offset) const {
    if ((offset + len) < input_.size()) {
      return false;
    }

    memcpy(buf, &input_[static_cast<size_t>(offset)], len);
    return true;
  }

 private:
  const std::vector<uint8_t>& input_;
};

class VectorWriter final : public zip_archive::Writer {
 public:
  VectorWriter() : Writer() {}

  bool Append(uint8_t* buf, size_t size) {
    output_.insert(output_.end(), buf, buf + size);
    return true;
  }

  std::vector<uint8_t>& GetOutput() { return output_; }

 private:
  std::vector<uint8_t> output_;
};

class BadReader final : public zip_archive::Reader {
 public:
  BadReader() : Reader() {}

  bool ReadAtOffset(uint8_t*, size_t, off64_t) const { return false; }
};

class BadWriter final : public zip_archive::Writer {
 public:
  BadWriter() : Writer() {}

  bool Append(uint8_t*, size_t) { return false; }
};

TEST(ziparchive, Inflate) {
  const uint32_t compressed_length = static_cast<uint32_t>(kATxtContentsCompressed.size());
  const uint32_t uncompressed_length = static_cast<uint32_t>(kATxtContents.size());

  const VectorReader reader(kATxtContentsCompressed);
  {
    VectorWriter writer;
    uint64_t crc_out = 0;

    int32_t ret =
        zip_archive::Inflate(reader, compressed_length, uncompressed_length, &writer, &crc_out);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(kATxtContents, writer.GetOutput());
    ASSERT_EQ(0x950821C5u, crc_out);
  }

  {
    VectorWriter writer;
    int32_t ret =
        zip_archive::Inflate(reader, compressed_length, uncompressed_length, &writer, nullptr);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(kATxtContents, writer.GetOutput());
  }

  {
    BadWriter writer;
    int32_t ret =
        zip_archive::Inflate(reader, compressed_length, uncompressed_length, &writer, nullptr);
    ASSERT_EQ(kIoError, ret);
  }

  {
    BadReader reader;
    VectorWriter writer;
    int32_t ret =
        zip_archive::Inflate(reader, compressed_length, uncompressed_length, &writer, nullptr);
    ASSERT_EQ(kIoError, ret);
    ASSERT_EQ(0u, writer.GetOutput().size());
  }
}

// The class constructs a zipfile with zip64 format, and test the parsing logic.
class Zip64ParseTest : public ::testing::Test {
 protected:
  struct LocalFileEntry {
    std::vector<uint8_t> local_file_header;
    std::string file_name;
    std::vector<uint8_t> extended_field;
    // Fake data to mimic the compressed bytes in the zipfile.
    std::vector<uint8_t> compressed_bytes;
    std::vector<uint8_t> data_descriptor;

    size_t GetSize() const {
      return local_file_header.size() + file_name.size() + extended_field.size() +
             compressed_bytes.size() + data_descriptor.size();
    }

    void CopyToOutput(std::vector<uint8_t>* output) const {
      std::copy(local_file_header.begin(), local_file_header.end(), std::back_inserter(*output));
      std::copy(file_name.begin(), file_name.end(), std::back_inserter(*output));
      std::copy(extended_field.begin(), extended_field.end(), std::back_inserter(*output));
      std::copy(compressed_bytes.begin(), compressed_bytes.end(), std::back_inserter(*output));
      std::copy(data_descriptor.begin(), data_descriptor.end(), std::back_inserter(*output));
    }
  };

  struct CdRecordEntry {
    std::vector<uint8_t> central_directory_record;
    std::string file_name;
    std::vector<uint8_t> extended_field;

    size_t GetSize() const {
      return central_directory_record.size() + file_name.size() + extended_field.size();
    }

    void CopyToOutput(std::vector<uint8_t>* output) const {
      std::copy(central_directory_record.begin(), central_directory_record.end(),
                std::back_inserter(*output));
      std::copy(file_name.begin(), file_name.end(), std::back_inserter(*output));
      std::copy(extended_field.begin(), extended_field.end(), std::back_inserter(*output));
    }
  };

  static void ConstructLocalFileHeader(const std::string& name, std::vector<uint8_t>* output,
                                       uint32_t uncompressed_size, uint32_t compressed_size) {
    LocalFileHeader lfh = {};
    lfh.lfh_signature = LocalFileHeader::kSignature;
    lfh.compressed_size = compressed_size;
    lfh.uncompressed_size = uncompressed_size;
    lfh.file_name_length = static_cast<uint16_t>(name.size());
    lfh.extra_field_length = 20;
    *output = std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&lfh),
                                   reinterpret_cast<uint8_t*>(&lfh) + sizeof(LocalFileHeader));
  }

  // Put one zip64 extended info in the extended field.
  static void ConstructExtendedField(const std::vector<uint64_t>& zip64_fields,
                                     std::vector<uint8_t>* output) {
    ASSERT_FALSE(zip64_fields.empty());
    uint16_t data_size = 8 * static_cast<uint16_t>(zip64_fields.size());
    std::vector<uint8_t> extended_field(data_size + 4);
    android::base::put_unaligned(extended_field.data(), Zip64ExtendedInfo::kHeaderId);
    android::base::put_unaligned(extended_field.data() + 2, data_size);
    size_t offset = 4;
    for (const auto& field : zip64_fields) {
      android::base::put_unaligned(extended_field.data() + offset, field);
      offset += 8;
    }

    *output = std::move(extended_field);
  }

  static void ConstructCentralDirectoryRecord(const std::string& name, uint32_t uncompressed_size,
                                              uint32_t compressed_size, uint32_t local_offset,
                                              std::vector<uint8_t>* output) {
    CentralDirectoryRecord cdr = {};
    cdr.record_signature = CentralDirectoryRecord::kSignature;
    cdr.compressed_size = uncompressed_size;
    cdr.uncompressed_size = compressed_size;
    cdr.file_name_length = static_cast<uint16_t>(name.size());
    cdr.extra_field_length = local_offset == UINT32_MAX ? 28 : 20;
    cdr.local_file_header_offset = local_offset;
    *output =
        std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&cdr),
                             reinterpret_cast<uint8_t*>(&cdr) + sizeof(CentralDirectoryRecord));
  }

  // Add an entry to the zipfile, construct the corresponding local header and cd entry.
  void AddEntry(const std::string& name, const std::vector<uint8_t>& content,
                bool uncompressed_size_in_extended, bool compressed_size_in_extended,
                bool local_offset_in_extended, bool include_data_descriptor = false) {
    auto uncompressed_size = static_cast<uint32_t>(content.size());
    auto compressed_size = static_cast<uint32_t>(content.size());
    uint32_t local_file_header_offset = 0;
    std::for_each(file_entries_.begin(), file_entries_.end(),
                  [&local_file_header_offset](const LocalFileEntry& file_entry) {
                    local_file_header_offset += file_entry.GetSize();
                  });

    std::vector<uint64_t> zip64_fields;
    if (uncompressed_size_in_extended) {
      zip64_fields.push_back(uncompressed_size);
      uncompressed_size = UINT32_MAX;
    }
    if (compressed_size_in_extended) {
      zip64_fields.push_back(compressed_size);
      compressed_size = UINT32_MAX;
    }
    LocalFileEntry local_entry = {
        .local_file_header = {},
        .file_name = name,
        .extended_field = {},
        .compressed_bytes = content,
    };
    ConstructLocalFileHeader(name, &local_entry.local_file_header, uncompressed_size,
                             compressed_size);
    ConstructExtendedField(zip64_fields, &local_entry.extended_field);
    if (include_data_descriptor) {
      size_t descriptor_size = compressed_size_in_extended ? 24 : 16;
      local_entry.data_descriptor.resize(descriptor_size);
      uint8_t* write_ptr = local_entry.data_descriptor.data();
      EmitUnaligned<uint32_t>(&write_ptr, DataDescriptor::kOptSignature);
      EmitUnaligned<uint32_t>(&write_ptr, 0 /* crc */);
      if (compressed_size_in_extended) {
        EmitUnaligned<uint64_t>(&write_ptr, compressed_size_in_extended);
        EmitUnaligned<uint64_t>(&write_ptr, uncompressed_size_in_extended);
      } else {
        EmitUnaligned<uint32_t>(&write_ptr, compressed_size_in_extended);
        EmitUnaligned<uint32_t>(&write_ptr, uncompressed_size_in_extended);
      }
    }

    file_entries_.push_back(std::move(local_entry));

    if (local_offset_in_extended) {
      zip64_fields.push_back(local_file_header_offset);
      local_file_header_offset = UINT32_MAX;
    }
    CdRecordEntry cd_entry = {
        .central_directory_record = {},
        .file_name = name,
        .extended_field = {},
    };
    ConstructCentralDirectoryRecord(name, uncompressed_size, compressed_size,
                                    local_file_header_offset, &cd_entry.central_directory_record);
    ConstructExtendedField(zip64_fields, &cd_entry.extended_field);
    cd_entries_.push_back(std::move(cd_entry));
  }

  void ConstructEocd() {
    ASSERT_EQ(file_entries_.size(), cd_entries_.size());
    Zip64EocdRecord zip64_eocd = {};
    zip64_eocd.record_signature = Zip64EocdRecord::kSignature;
    zip64_eocd.num_records = file_entries_.size();
    zip64_eocd.cd_size = 0;
    std::for_each(
        cd_entries_.begin(), cd_entries_.end(),
        [&zip64_eocd](const CdRecordEntry& cd_entry) { zip64_eocd.cd_size += cd_entry.GetSize(); });
    zip64_eocd.cd_start_offset = 0;
    std::for_each(file_entries_.begin(), file_entries_.end(),
                  [&zip64_eocd](const LocalFileEntry& file_entry) {
                    zip64_eocd.cd_start_offset += file_entry.GetSize();
                  });
    zip64_eocd_record_ =
        std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&zip64_eocd),
                             reinterpret_cast<uint8_t*>(&zip64_eocd) + sizeof(Zip64EocdRecord));

    Zip64EocdLocator zip64_locator = {};
    zip64_locator.locator_signature = Zip64EocdLocator::kSignature;
    zip64_locator.zip64_eocd_offset = zip64_eocd.cd_start_offset + zip64_eocd.cd_size;
    zip64_eocd_locator_ =
        std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&zip64_locator),
                             reinterpret_cast<uint8_t*>(&zip64_locator) + sizeof(Zip64EocdLocator));

    EocdRecord eocd = {};
    eocd.eocd_signature = EocdRecord::kSignature,
    eocd.num_records = file_entries_.size() > UINT16_MAX
                           ? UINT16_MAX
                           : static_cast<uint16_t>(file_entries_.size());
    eocd.cd_size = UINT32_MAX;
    eocd.cd_start_offset = UINT32_MAX;
    eocd_record_ = std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&eocd),
                                        reinterpret_cast<uint8_t*>(&eocd) + sizeof(EocdRecord));
  }

  // Concatenate all the local file entries, cd entries, and eocd metadata.
  void ConstructZipFile() {
    for (const auto& file_entry : file_entries_) {
      file_entry.CopyToOutput(&zip_content_);
    }
    for (const auto& cd_entry : cd_entries_) {
      cd_entry.CopyToOutput(&zip_content_);
    }
    std::copy(zip64_eocd_record_.begin(), zip64_eocd_record_.end(),
              std::back_inserter(zip_content_));
    std::copy(zip64_eocd_locator_.begin(), zip64_eocd_locator_.end(),
              std::back_inserter(zip_content_));
    std::copy(eocd_record_.begin(), eocd_record_.end(), std::back_inserter(zip_content_));
  }

  std::vector<uint8_t> zip_content_;

  std::vector<LocalFileEntry> file_entries_;
  std::vector<CdRecordEntry> cd_entries_;
  std::vector<uint8_t> zip64_eocd_record_;
  std::vector<uint8_t> zip64_eocd_locator_;
  std::vector<uint8_t> eocd_record_;
};

TEST_F(Zip64ParseTest, openFile) {
  AddEntry("a.txt", std::vector<uint8_t>(100, 'a'), true, true, false);
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));
  CloseArchive(handle);
}

TEST_F(Zip64ParseTest, openFilelocalOffsetInExtendedField) {
  AddEntry("a.txt", std::vector<uint8_t>(100, 'a'), true, true, true);
  AddEntry("b.txt", std::vector<uint8_t>(200, 'b'), true, true, true);
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));
  CloseArchive(handle);
}

TEST_F(Zip64ParseTest, openFileCompressedNotInExtendedField) {
  AddEntry("a.txt", std::vector<uint8_t>(100, 'a'), true, false, false);
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  // Zip64 extended fields must include both uncompressed and compressed size.
  ASSERT_NE(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));
  CloseArchive(handle);
}

TEST_F(Zip64ParseTest, findEntry) {
  AddEntry("a.txt", std::vector<uint8_t>(200, 'a'), true, true, true);
  AddEntry("b.txt", std::vector<uint8_t>(300, 'b'), true, true, false);
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));
  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, "a.txt", &entry));
  ASSERT_EQ(200, entry.uncompressed_length);
  ASSERT_EQ(200, entry.compressed_length);

  ASSERT_EQ(0, FindEntry(handle, "b.txt", &entry));
  ASSERT_EQ(300, entry.uncompressed_length);
  ASSERT_EQ(300, entry.compressed_length);
  CloseArchive(handle);
}

TEST_F(Zip64ParseTest, dataDescriptor) {
  AddEntry("a.txt", std::vector<uint8_t>(200, 'a'), true, true, true, false);
  AddEntry("b.txt", std::vector<uint8_t>(300, 'b'), true, true, true, false);
  // We want a file with compressed size in extended fields, but
  // data descriptor still in 32 bit values.
  auto& local_entry = file_entries_.back();
  local_entry.data_descriptor.resize(16);
  uint8_t* write_ptr = local_entry.data_descriptor.data();
  EmitUnaligned<uint32_t>(&write_ptr, DataDescriptor::kOptSignature);
  EmitUnaligned<uint32_t>(&write_ptr, 0 /* crc */);
  EmitUnaligned<uint32_t>(&write_ptr, 300);
  EmitUnaligned<uint32_t>(&write_ptr, 300);

  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(),
                                     "debug_zip64", &handle));
  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, "a.txt", &entry));
  ASSERT_EQ(200, entry.uncompressed_length);
  ASSERT_EQ(200, entry.compressed_length);

  ASSERT_EQ(0, FindEntry(handle, "b.txt", &entry));
  ASSERT_EQ(300, entry.uncompressed_length);
  ASSERT_EQ(300, entry.compressed_length);
  CloseArchive(handle);
}

TEST_F(Zip64ParseTest, openFileIncorrectDataSizeInLocalExtendedField) {
  AddEntry("a.txt", std::vector<uint8_t>(100, 'a'), true, true, false);
  ASSERT_EQ(1, file_entries_.size());
  auto& extended_field = file_entries_[0].extended_field;
  // data size exceeds the extended field size in local header.
  android::base::put_unaligned<uint16_t>(extended_field.data() + 2, 30);
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));
  ZipEntry64 entry;
  ASSERT_NE(0, FindEntry(handle, "a.txt", &entry));

  CloseArchive(handle);
}

TEST_F(Zip64ParseTest, iterates) {
  std::set<std::string_view> names{"a.txt", "b.txt", "c.txt", "d.txt", "e.txt"};
  for (const auto& name : names) {
    AddEntry(std::string(name), std::vector<uint8_t>(100, name[0]), true, true, true);
  }
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));

  void* iteration_cookie;
  ASSERT_EQ(0, StartIteration(handle, &iteration_cookie));
  std::set<std::string_view> result;
  std::string_view name;
  ZipEntry64 entry;
  while (Next(iteration_cookie, &entry, &name) == 0) result.emplace(name);
  ASSERT_EQ(names, result);

  CloseArchive(handle);
}

TEST_F(Zip64ParseTest, zip64EocdWrongLocatorOffset) {
  AddEntry("a.txt", std::vector<uint8_t>(1, 'a'), true, true, true);
  ConstructEocd();
  zip_content_.resize(20, 'a');
  std::copy(zip64_eocd_locator_.begin(), zip64_eocd_locator_.end(),
            std::back_inserter(zip_content_));
  std::copy(eocd_record_.begin(), eocd_record_.end(), std::back_inserter(zip_content_));

  ZipArchiveHandle handle;
  ASSERT_NE(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));
  CloseArchive(handle);
}

TEST_F(Zip64ParseTest, extract) {
  std::vector<uint8_t> content(200, 'a');
  AddEntry("a.txt", content, true, true, true);
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));
  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, "a.txt", &entry));

  VectorWriter writer;
  ASSERT_EQ(0, ExtractToWriter(handle, &entry, &writer));
  ASSERT_EQ(content, writer.GetOutput());
}

TEST_F(Zip64ParseTest, extractWithDataDescriptor) {
  std::vector<uint8_t> content(300, 'b');
  AddEntry("a.txt", std::vector<uint8_t>(200, 'a'), true, true, true);
  AddEntry("b.txt", content, true, true, true, true /* data descriptor */);
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(
      0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(), "debug_zip64", &handle));
  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, "b.txt", &entry));

  VectorWriter writer;
  ASSERT_EQ(0, ExtractToWriter(handle, &entry, &writer));
  ASSERT_EQ(content, writer.GetOutput());
}

TEST_F(Zip64ParseTest, extraLFHOffset) {
  std::vector<uint8_t> content(300, 'b');
  AddEntry("a.txt", std::vector<uint8_t>(200, 'a'), true, true, true);
  AddEntry("b.txt", content, true, true, true, true /* data descriptor */);

  ASSERT_EQ(cd_entries_.back().extended_field.size(), 4 + 8 * 3)
      << "Extended field should contain 2 bytes id, 2 bytes size, and 3 "
         "values, each 64 bit";
  uint32_t local_file_header_offset = 0;
  std::for_each(file_entries_.begin(), file_entries_.end() - 1,
                [&local_file_header_offset](const LocalFileEntry& file_entry) {
                  local_file_header_offset += file_entry.GetSize();
                });
  auto& cd_entry = cd_entries_.back();
  // We want to construct a central directory record with LFH < 0xFFFFFFFF
  // but still comes with a 64 bit LFH in extended field.
  ConstructCentralDirectoryRecord(
      "b.txt", static_cast<uint32_t>(content.size()),
      static_cast<uint32_t>(content.size()), local_file_header_offset,
      &cd_entry.central_directory_record);
  ConstructEocd();
  ConstructZipFile();

  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFromMemory(zip_content_.data(), zip_content_.size(),
                                     "debug_zip64", &handle));
  ZipEntry64 entry;
  ASSERT_EQ(0, FindEntry(handle, "b.txt", &entry));

  VectorWriter writer;
  ASSERT_EQ(0, ExtractToWriter(handle, &entry, &writer));
  ASSERT_EQ(content, writer.GetOutput());
}

TEST(ziparchive, Bug174945959) {
  static const std::vector<uint8_t> zip {
    0x50, 0x4b, 0x03, 0x04, 0x50, 0x4b, 0x01, 0x02, 0x01, 0x53, 0x46, 0x5b,
    0xa4, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x7f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x03, 0x12, 0x00, 0x07, 0x00, 0x00, 0x3b, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0xa4, 0x2e, 0x00, 0x00, 0x00,
    0x24, 0x24, 0xb6, 0x3f, 0xff, 0xff, 0x31, 0x51, 0x49, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x4b, 0x05, 0x50, 0x4b, 0x05, 0x06, 0xc5, 0x1f, 0x4a, 0x04, 0x00, 0x21,
    0x01, 0x00, 0x6a, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
  ZipArchiveHandle handle;
  ASSERT_EQ(0, OpenArchiveFromMemory(&zip[0], zip.size(), "name", &handle));

  void* cookie;
  ASSERT_EQ(0, StartIteration(handle, &cookie));
  ZipEntry ze;
  std::string name;
  int result;
  while ((result = Next(cookie, &ze, &name)) == 0) {
  }
  EndIteration(cookie);
}
