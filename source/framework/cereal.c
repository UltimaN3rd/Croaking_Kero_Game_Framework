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

bool cereal_WriteToFile (const cereal_t cereal[], const int cereal_count, FILE *file) {
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
						case cereal_string: fprintf (file, " %lu %s", strlen((const char*)cereal[i].var), (const char*)cereal[i].var); break;
					}
				}
			} break;
			case cereal_bool: fprintf (file, "%"PRIu8, *(bool*)cereal[i].var ? 1 : 0); break;
			case cereal_u8: fprintf (file, "%"PRIu8, *(uint8_t*)cereal[i].var); break;
			case cereal_u16: fprintf (file, "%"PRIu16, *(uint16_t*)cereal[i].var); break;
			case cereal_u32: fprintf (file, "%"PRIu32, *(uint32_t*)cereal[i].var); break;
			case cereal_u64: fprintf (file, "%"PRIu64, *(uint64_t*)cereal[i].var); break;
			case cereal_f32: fprintf (file, "%f", *(float*)cereal[i].var); break;
			case cereal_string: fprintf (file, "%lu %s", strlen((const char*)cereal[i].var), (const char*)cereal[i].var); break;
		}
		fprintf (file, "]\n");
	}
    return true;
}

bool cereal_ReadFromFile (const cereal_t cereal[], const int cereal_count, FILE *file) {
	char buf[1024];
	while (fgets (buf, sizeof (buf), file)) {
		size_t linelen = strlen (buf);
		const char *key = buf;
		char *c = buf;
		while (*c != '[' && (c-buf) < linelen) ++c;
		--c;
		if (c <= buf) {
			LOG ("Game save file [%s] corrupted. Line: [%s]", update_data.game_save_filename, buf);
			return false;
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
			LOG ("Game save file [%s] corrupted. Key [%s] invalid.", update_data.game_save_filename, key);
			return false;
		}
		*c = ' '; // repair original string
		c += 2; // advance to data value. Already checked above that at least the last two characters were valid, so worst case we're on a NULL
		int scan_result = 0;
		switch (cer.type) {
			case cereal_bool: {uint8_t temp = 0; scan_result = sscanf (c, "%"SCNu8, &temp); *(bool*)cer.var = temp; } break;
			case cereal_u8: scan_result = sscanf (c, "%"SCNu8, (uint8_t*)cer.var); break;
			case cereal_u16: scan_result = sscanf (c, "%"SCNu16, (uint16_t*)cer.var); break;
			case cereal_u32: scan_result = sscanf (c, "%"SCNu32, (uint32_t*)cer.var); break;
			case cereal_u64: scan_result = sscanf (c, "%"SCNu64, (uint64_t*)cer.var); break;
			case cereal_f32: scan_result = sscanf (c, "%f", (float*)cer.var); break;
			case cereal_string: {
				uint64_t string_length;
				int strlen_chars_read = 0;
				scan_result = sscanf (c, "%"SCNu64"%n", &string_length, &strlen_chars_read);
				if (scan_result != 1) {
					LOG ("Game save file [%s] corrupted. Failed to read first value on line [%s]", update_data.game_save_filename, buf);
					return false;
				}
				if (string_length == 0) {
					((char *)cer.var)[0] = 0;
				}
				else {
					if (string_length > cer.u.string.capacity) {
						LOG ("Game save file [%s] corrupted. String length [%"PRIu64"] exceeds capacity [%"PRIu16"]. Line [%s]", update_data.game_save_filename, string_length, cer.u.string.capacity, buf);
						return false;
					}
					else {
						((char*)cer.var)[string_length] = 0;
						c += strlen_chars_read;
						if (*c != ' ') {
							LOG ("Game save file [%s] corrupted. String length is not followed by a space. Line [%s]", update_data.game_save_filename, buf);
							return false;
						}
						++c;
						uint64_t chars_copied = 0;
						for (; chars_copied < string_length; ++chars_copied) {
							char ch = c[chars_copied];
							if (ch == 0) {
								LOG ("Game save file [%s] corrupted. String length [%"PRIu64"] but only read [%"PRIi64"]. Line [%s]", update_data.game_save_filename, string_length, chars_copied, buf);
								return false;
							}
							((char*)cer.var)[chars_copied] = ch;
						}
						if (chars_copied != string_length) {
							LOG ("Game save file [%s] corrupted. String length [%"PRIu64"] but only read [%"PRIi64"]. Line [%s]", update_data.game_save_filename, string_length, chars_copied, buf);
							return false;
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
					case cereal_u8: scan_result = sscanf (c, "%"SCNu8"%n", (uint8_t*)cer.u.array.counter, &chars_read); count = *(uint8_t*)cer.u.array.counter; break;
					case cereal_u16: scan_result = sscanf (c, "%"SCNu16"%n", (uint16_t*)cer.u.array.counter, &chars_read); count = *(uint16_t*)cer.u.array.counter; break;
					case cereal_u32: scan_result = sscanf (c, "%"SCNu32"%n", (uint32_t*)cer.u.array.counter, &chars_read); count = *(uint32_t*)cer.u.array.counter; break;
					case cereal_u64: scan_result = sscanf (c, "%"SCNu64"%n", (uint64_t*)cer.u.array.counter, &chars_read); count = *(uint64_t*)cer.u.array.counter; break;
					case cereal_bool:
					case cereal_f32:
					case cereal_string:
					case cereal_array: __builtin_unreachable();
				}
				c += chars_read;
				if (c - buf > linelen || scan_result != 1) {
					LOG ("Game save file [%s] corrupted. Reached end of line before reading all array values on line [%s]", update_data.game_save_filename, buf);
					return false;
				}
				char *arr = (char*)cer.var;
				for (uint64_t i = 0; i < count; ++i) {
					switch (cer.u.array.type) {
						case cereal_bool: {uint8_t temp = 0; scan_result = sscanf (c, "%"SCNu8, &temp); *(bool*)arr = temp; arr += sizeof (bool); } break;
						case cereal_u8: scan_result = sscanf (c, " %"SCNu8"%n", (uint8_t*)arr, &chars_read); arr += sizeof (uint8_t); break;
						case cereal_u16: scan_result = sscanf (c, " %"SCNu16"%n", (uint16_t*)arr, &chars_read); arr += sizeof (uint16_t); break;
						case cereal_u32: scan_result = sscanf (c, " %"SCNu32"%n", (uint32_t*)arr, &chars_read); arr += sizeof (uint32_t); break;
						case cereal_u64: scan_result = sscanf (c, " %"SCNu64"%n", (uint64_t*)arr, &chars_read); arr += sizeof (uint64_t); break;
						case cereal_f32: scan_result = sscanf (c, " %f%n", (float*)arr, &chars_read); arr += sizeof (float); break;
						case cereal_string: { // UNTESTED
							uint64_t string_length;
							int strlen_chars_read = 0;
							scan_result = sscanf (c, " %"SCNu64"%n", &string_length, &strlen_chars_read);
							if (scan_result != 1) {
								LOG ("Game save file [%s] corrupted. Failed to read first value on line [%s]", update_data.game_save_filename, buf);
								return false;
							}
							if (string_length == 0) {
								((char *)cer.var)[0] = 0;
							}
							else {
								if (string_length > cer.u.string.capacity) {
									LOG ("Game save file [%s] corrupted. String length [%"PRIu64"] exceeds capacity [%"PRIu16"]. Line [%s]", update_data.game_save_filename, string_length, cer.u.string.capacity, buf);
									return false;
								}
								else {
									((char*)cer.var)[string_length] = 0;
									c += strlen_chars_read;
									if (*c != ' ') {
										LOG ("Game save file [%s] corrupted. String length is not followed by a space. Line [%s]", update_data.game_save_filename, buf);
										return false;
									}
									++c;
									uint64_t chars_copied = 0;
									for (; chars_copied < string_length; ++chars_copied) {
										char ch = c[chars_copied];
										if (ch == 0) {
											LOG ("Game save file [%s] corrupted. String length [%"PRIu64"] but only read [%"PRIi64"]. Line [%s]", update_data.game_save_filename, string_length, chars_copied, buf);
											return false;
										}
										((char*)cer.var)[chars_copied] = ch;
									}
									if (chars_copied != string_length) {
										LOG ("Game save file [%s] corrupted. String length [%"PRIu64"] but only read [%"PRIi64"]. Line [%s]", update_data.game_save_filename, string_length, chars_copied, buf);
										return false;
									}
									chars_read += 1 + string_length;
								}
							}
						} break;
						case cereal_array: __builtin_unreachable();
					}
					c += chars_read;
					if (c - buf > linelen || scan_result != 1) {
						LOG ("Game save file [%s] corrupted. Reached end of line before reading all array values on line [%s]", update_data.game_save_filename, buf);
						return false;
					}
				}
			}
		}
		if (scan_result != 1) {
			LOG ("Game save file [%s] corrupted. Failed to read first value on line [%s]", update_data.game_save_filename, buf);
			return false;
		}
	}
	if (feof (file)) {
		return true;
	}
	else {
		return false;
	}
}
