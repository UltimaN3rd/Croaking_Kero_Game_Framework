// Copyright [2025] [Nicholas Walton]
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cereal.h"
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include "update.h"

extern update_data_t update_data;
uint64_t cereal_dummy;

bool cereal_WriteToFile (const cereal_t cereal[], const int cereal_count, const char *const filename) {
	FILE *file = fopen (filename, "wb");
	assert (file); if (!file) { LOG ("Failed to open file [%s]", filename); return false; }
	defer { fclose (file); }
	int items_written = 0;
	for (int i = 0; i < cereal_count; ++i) {
		fprintf (file, "%s [", cereal[i].key);
		switch (cereal[i].type) {
			case cereal_array: {
				uint64_t count = 0;
				assert (cereal[i].u.array.counter_type != cereal_array);
				assert (cereal[i].u.array.counter_type != cereal_f32);
				assert (cereal[i].u.array.counter_type != cereal_bool);
				assert (cereal[i].u.array.counter_type != cereal_string);
				switch (cereal[i].u.array.counter_type) {
					case cereal_f32:
					case cereal_bool:
					case cereal_string:
					case cereal_array: unreachable();
					case cereal_u8: fprintf (file, "%"PRIu8, *(uint8_t*)cereal[i].u.array.counter); count = *(uint8_t*)cereal[i].u.array.counter; break;
					case cereal_u16: fprintf (file, "%"PRIu16, *(uint16_t*)cereal[i].u.array.counter); count = *(uint16_t*)cereal[i].u.array.counter; break;
					case cereal_u32: fprintf (file, "%"PRIu32, *(uint32_t*)cereal[i].u.array.counter); count = *(uint32_t*)cereal[i].u.array.counter; break;
					case cereal_u64: fprintf (file, "%"PRIu64, *(uint64_t*)cereal[i].u.array.counter); count = *(uint64_t*)cereal[i].u.array.counter; break;
				}
				assert (count <= cereal[i].u.array.capacity);
				if (count > cereal[i].u.array.capacity) {
					LOG ("Item [%d] array count [%"PRIu64"] greater than capacity [%d]", i, count, cereal[i].u.array.capacity);
					continue;
				}
				char *v = (char*)cereal[i].var;
				for (int j = 0; j < count; ++j) {
					switch (cereal[i].u.array.type) {
						case cereal_array: unreachable ();
						case cereal_bool: fprintf (file, " %"PRIu8, *(bool*)v ? 1 : 0); v += sizeof (bool); break;
						case cereal_u8: fprintf (file, " %"PRIu8, *(uint8_t*)v); v += sizeof (uint8_t); break;
						case cereal_u16: fprintf (file, " %"PRIu16, *(uint16_t*)v); v += sizeof (uint16_t); break;
						case cereal_u32: fprintf (file, " %"PRIu32, *(uint32_t*)v); v += sizeof (uint32_t); break;
						case cereal_u64: fprintf (file, " %"PRIu64, *(uint64_t*)v); v += sizeof (uint64_t); break;
						case cereal_f32: fprintf (file, " %f", *(float*)v); v += sizeof (float); break;
						case cereal_string: fprintf (file, " %zu %s", strlen((const char*)cereal[i].var), (const char*)cereal[i].var); break;
					}
				}
			} break;
			case cereal_bool: fprintf (file, "%"PRIu8, *(bool*)cereal[i].var ? 1 : 0); break;
			case cereal_u8: fprintf (file, "%"PRIu8, *(uint8_t*)cereal[i].var); break;
			case cereal_u16: fprintf (file, "%"PRIu16, *(uint16_t*)cereal[i].var); break;
			case cereal_u32: fprintf (file, "%"PRIu32, *(uint32_t*)cereal[i].var); break;
			case cereal_u64: fprintf (file, "%"PRIu64, *(uint64_t*)cereal[i].var); break;
			case cereal_f32: fprintf (file, "%f", *(float*)cereal[i].var); break;
			case cereal_string: fprintf (file, "%zu %s", strlen((const char*)cereal[i].var), (const char*)cereal[i].var); break;
		}
		fprintf (file, "]\n");
	}
    return true;
}

int cereal_ReadFromFile (const cereal_t cereal[], const int cereal_count, const char *const filename) {
	FILE *file = fopen (filename, "rb");
	if (!file) { LOG ("Failed to read file [%s]", filename); return false; }
	defer { fclose (file); }
	long start_pos = ftell (file);
	fseek (file, 0, SEEK_END);
	long end_pos = ftell (file);
	fseek (file, start_pos, SEEK_SET);
	uint32_t size = end_pos - start_pos;
	char *buf = malloc (size);
	assert (buf); if (buf == NULL) { LOG ("[%s] Failed to allocate %"PRIu32" bytes", filename, size); return false; }
	defer { free (buf); }
	fread (buf, 1, size, file);
	char *c = buf;
	int items_read = 0;
	while (c < buf + size) {
		__label__ goto_nextline;
		goto_nextline:
		const char *const line_start = c;
		char *line_end = c;
		uint32_t linelen = 0;
		{
			while (line_end < buf + size && *line_end != '\n') ++line_end;
			linelen = line_end - c;
			if (line_end < buf + size - 1) *line_end = '\0';
		}
		const char *key = c;
		while (*c != '[' && c < line_end) ++c;
		--c;
		if (c <= line_start) {
			LOG ("[%s] Skipping line: [%s]", filename, line_start);
			c = line_end+1;
			goto goto_nextline;
		}
		*c = 0; // add NULL to end of key string
		cereal_t cer;
		bool found = false;
		for (int i = 0; i < cereal_count; ++i) {
			if (strcmp (key, cereal[i].key) == 0) {
				cer = cereal[i];
				found = true;
				break;
			}
		}
		if (!found) {
			LOG ("[%s] Skipping invalid key [%s]", filename, key);
			c = line_end+1;
			goto goto_nextline;
		}
		*c = ' '; // repair original string
		c += 2; // advance to data value. Already checked above that at least the last two characters were valid, so worst case we're on a NULL
		int scan_result = 0;
		int scanned_chars = 0;
		switch (cer.type) {
			case cereal_bool: {uint8_t temp = 0; scan_result = sscanf (c, "%"SCNu8"%n", &temp, &scanned_chars); *(bool*)cer.var = temp; c += scanned_chars; } break;
			case cereal_u8: scan_result = sscanf (c, "%"SCNu8"%n", (uint8_t*)cer.var, &scanned_chars); c += scanned_chars; break;
			case cereal_u16: scan_result = sscanf (c, "%"SCNu16"%n", (uint16_t*)cer.var, &scanned_chars); c += scanned_chars; break;
			case cereal_u32: scan_result = sscanf (c, "%"SCNu32"%n", (uint32_t*)cer.var, &scanned_chars); c += scanned_chars; break;
			case cereal_u64: scan_result = sscanf (c, "%"SCNu64"%n", (uint64_t*)cer.var, &scanned_chars); c += scanned_chars; break;
			case cereal_f32: scan_result = sscanf (c, "%f%n", (float*)cer.var, &scanned_chars); c += scanned_chars; break;
			case cereal_string: {
				uint64_t string_length;
				scan_result = sscanf (c, "%"SCNu64"%n", &string_length, &scanned_chars); c += scanned_chars;
				if (scan_result != 1) {
					LOG ("[%s] Failed to read first value on line [%s]", filename, line_start);
					c = line_end+1;
					goto goto_nextline;
				}
				if (string_length == 0) {
					((char *)cer.var)[0] = 0;
				}
				else {
					if (string_length > cer.u.string.capacity) {
						LOG ("[%s] String length [%"PRIu64"] exceeds capacity [%"PRIu16"]. Line [%s]", filename, string_length, cer.u.string.capacity, line_start);
						c = line_end+1;
						goto goto_nextline;
					}
					else {
						((char*)cer.var)[string_length] = 0;
						if (*c != ' ') {
							LOG ("[%s] String length is not followed by a space. Line [%s]", filename, line_start);
							c = line_end+1;
							goto goto_nextline;
						}
						++c;
						uint64_t chars_copied = 0;
						for (; chars_copied < string_length; ++chars_copied) {
							char ch = c[chars_copied];
							if (ch == 0) {
								LOG ("[%s] String length [%"PRIu64"] but only read [%"PRIi64"]. Line [%s]", filename, string_length, chars_copied, line_start);
								c = line_end+1;
								goto goto_nextline;
							}
							((char*)cer.var)[chars_copied] = ch;
						}
						if (chars_copied != string_length) {
							LOG ("[%s] String length [%"PRIu64"] but only read [%"PRIi64"]. Line [%s]", filename, string_length, chars_copied, line_start);
							c = line_end+1;
							goto goto_nextline;
						}
					}
				}
			} break;
			case cereal_array: {
				uint64_t count = 0;
				assert (cer.u.array.counter_type != cereal_array);
				assert (cer.u.array.counter_type != cereal_f32);
				assert (cer.u.array.counter_type != cereal_bool);
				assert (cer.u.array.counter_type != cereal_string);
				assert (cer.u.array.type != cereal_array);
				int chars_read = 0;
				switch (cer.u.array.counter_type) {
					case cereal_u8: scan_result = sscanf (c, "%"SCNu8" %n", (uint8_t*)cer.u.array.counter, &chars_read); count = *(uint8_t*)cer.u.array.counter; c += chars_read; break;
					case cereal_u16: scan_result = sscanf (c, "%"SCNu16" %n", (uint16_t*)cer.u.array.counter, &chars_read); count = *(uint16_t*)cer.u.array.counter; c += chars_read; break;
					case cereal_u32: scan_result = sscanf (c, "%"SCNu32" %n", (uint32_t*)cer.u.array.counter, &chars_read); count = *(uint32_t*)cer.u.array.counter; c += chars_read; break;
					case cereal_u64: scan_result = sscanf (c, "%"SCNu64" %n", (uint64_t*)cer.u.array.counter, &chars_read); count = *(uint64_t*)cer.u.array.counter; c += chars_read; break;
					case cereal_bool:
					case cereal_f32:
					case cereal_string:
					case cereal_array: __builtin_unreachable();
				}
				if (c >= line_end || scan_result != 1) {
					LOG ("[%s] Reached end of line before reading all array values on line [%s]", filename, line_start);
					c = line_end+1;
					goto goto_nextline;
				}
				char *arr = (char*)cer.var;
				for (uint64_t i = 0; i < count; ++i) {
					switch (cer.u.array.type) {
						case cereal_bool: {uint8_t temp = 0; scan_result = sscanf (c, "%"SCNu8"%n", &temp, &chars_read); *(bool*)arr = temp; arr += sizeof (bool); c += chars_read; } break;
						case cereal_u8: scan_result = sscanf (c, " %"SCNu8" %n", (uint8_t*)arr, &chars_read); arr += sizeof (uint8_t); c += chars_read; break;
						case cereal_u16: scan_result = sscanf (c, " %"SCNu16" %n", (uint16_t*)arr, &chars_read); arr += sizeof (uint16_t); c += chars_read; break;
						case cereal_u32: scan_result = sscanf (c, " %"SCNu32" %n", (uint32_t*)arr, &chars_read); arr += sizeof (uint32_t); c += chars_read; break;
						case cereal_u64: scan_result = sscanf (c, " %"SCNu64" %n", (uint64_t*)arr, &chars_read); arr += sizeof (uint64_t); c += chars_read; break;
						case cereal_f32: scan_result = sscanf (c, " %f %n", (float*)arr, &chars_read); arr += sizeof (float); c += chars_read; break;
						case cereal_string: { // UNTESTED
							uint64_t string_length;
							scan_result = sscanf (c, " %"SCNu64"%n", &string_length, &chars_read); c += chars_read;
							if (scan_result != 1) {
								LOG ("[%s] Failed to read first value on line [%s]", filename, line_start);
								c = line_end+1;
								goto goto_nextline;
							}
							if (string_length == 0) {
								((char *)cer.var)[0] = 0;
							}
							else {
								if (string_length > cer.u.string.capacity) {
									LOG ("[%s] String length [%"PRIu64"] exceeds capacity [%"PRIu16"]. Line [%s]", filename, string_length, cer.u.string.capacity, line_start);
									c = line_end+1;
									goto goto_nextline;
								}
								else {
									((char*)cer.var)[string_length] = 0;
									if (*c != ' ') {
										LOG ("[%s] String length is not followed by a space. Line [%s]", filename, line_start);
										c = line_end+1;
										goto goto_nextline;
									}
									++c;
									uint64_t chars_copied = 0;
									for (; chars_copied < string_length; ++chars_copied) {
										char ch = c[chars_copied];
										if (ch == 0) {
											LOG ("[%s] String length [%"PRIu64"] but only read [%"PRIi64"]. Line [%s]", filename, string_length, chars_copied, line_start);
											c = line_end+1;
											goto goto_nextline;
										}
										((char*)cer.var)[chars_copied] = ch;
									}
									if (chars_copied != string_length) {
										LOG ("[%s] String length [%"PRIu64"] but only read [%"PRIi64"]. Line [%s]", filename, string_length, chars_copied, line_start);
										c = line_end+1;
										goto goto_nextline;
									}
									chars_read += 1 + string_length;
								}
							}
						} break;
						case cereal_array: __builtin_unreachable();
					}
					if (c > line_end || scan_result != 1) {
						LOG ("[%s] Reached end of line before reading all array values on line [%s]", filename, line_start);
						c = line_end+1;
						goto goto_nextline;
					}
				}
			}
		}
		if (scan_result != 1) {
			LOG ("[%s] Failed to read first value on line [%s]", filename, line_start);
			c = line_end+1;
			goto goto_nextline;
		}
		c = line_end+1;
		++items_read;
	}
	return items_read;
}
