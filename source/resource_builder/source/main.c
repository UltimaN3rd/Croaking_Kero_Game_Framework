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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STRINGIFY___2(x) #x
#define STRINGIFY___(x) STRINGIFY___2(x)

#define PRINT_ERROR(...) do { printf("ERROR %s() "__FILE__" Line "STRINGIFY___(__LINE__)": ", __FUNCTION__); printf(__VA_ARGS__); printf("\n"); } while (0)

#define PRINT_ERROR_IF(a) do { if (a) PRINT_ERROR ("%s", #a); } while (0)

// Pass this the expression to evaluate
#define EXIT_IF(a) do { if(a) { PRINT_ERROR(#a); abort(); } } while(0)

#if DEBUG
#define DEBUG_ABORT(...) PRINT_ERROR (__VA_ARGS__); abort ()
#else
#define DEBUG_ABORT(...) PRINT_ERROR (__VA_ARGS__)
#endif

#include "utilities.h"
#include "folders.h"
#include "framework_types.h"

bool LoadCursor (const char *const folder, const char *const codename);

uint8_t palette[256][3];

FILE *phil, *header;

char current_directory[2048] = "";

typedef struct {
	union { int width,  w; };
	union { int height, h; };
	uint8_t *p;
} resources_sprite_t;

#define BITMAP_FONT_FIRST_VISIBLE_CHAR 33
#define BITMAP_FONT_LAST_VISIBLE_CHAR 126
#define BITMAP_FONT_NUM_VISIBLE_CHARS (BITMAP_FONT_LAST_VISIBLE_CHAR - BITMAP_FONT_FIRST_VISIBLE_CHAR + 1)

typedef struct {
    int line_height;
    int space_width;
    uint8_t baseline;
    uint8_t *pixels;
    struct {int w, h, offset;} bitmaps[BITMAP_FONT_NUM_VISIBLE_CHARS];
    int8_t descent[BITMAP_FONT_NUM_VISIBLE_CHARS];
} resources_font_t;

static bool LoadMusic (const char *filename);
resources_sprite_t sprite_LoadBMP (const char *filename);
bool font_Load (resources_font_t *font, char *directory);
char *ReadEntireFile (char *filename, int *return_file_length);

void PopDir() {
	char *slash = &current_directory[strlen(current_directory) - 2];
	while (slash > current_directory && *slash != '/')
		--slash;
	++slash;
	if (slash > current_directory)
		*slash = 0;
	chdir("../");
}


#include "sound.h"
#include "midi.c"

typedef enum { V1_waveform_sine, V1_waveform_triangle, V1_waveform_saw, V1_waveform_square, V1_waveform_noise } V1_waveform_e;
#define V1_WAVEFORM_COUNT (V1_waveform_noise + 1)

const char *const waveform_to_string[] = { [sound_waveform_none] = "sound_waveform_none", [sound_waveform_sine] = "sound_waveform_sine", [sound_waveform_triangle] = "sound_waveform_triangle", [sound_waveform_saw] = "sound_waveform_saw", [sound_waveform_pulse] = "sound_waveform_pulse", [sound_waveform_noise] = "sound_waveform_noise", [sound_waveform_silence] = "sound_waveform_silence", [sound_waveform_preparing] = "sound_waveform_preparing" };

// Max 8 full beats per pattern, split into 8ths
#define V1_EIGHTHS_PER_PATTERN_MAX 64
#define V1_PATTERNS_PER_TRACK 128

typedef struct {
	V1_waveform_e waveform;
	ADSRf_t ADSR;
    int16_t sweep;
    vibrato_t vibrato;
    uint32_t placeholder_color;
    int8_t square_duty_cycle, square_duty_cycle_sweep;
    char name[32];
} V1_instrument_t;

typedef struct {
    uint8_t note;
    uint8_t eighths;
    uint8_t instrument_override;
    bool lead_in, lead_out;
} V1_beat_t;

typedef struct {
    bool in_use;
    char name[17];
    uint32_t color;
    V1_beat_t beats[V1_EIGHTHS_PER_PATTERN_MAX];
} V1_pattern_t;

typedef struct {
    uint8_t pattern_current;
    uint8_t instrument;
    uint8_t volume;
    uint8_t patterns[V1_PATTERNS_PER_TRACK];
} V2_track_t;

typedef struct {
    struct {
        int current;
        V2_track_t track[6];
    } tracks;

    // Pattern 0 is invalid
    struct {
        uint16_t count;
        uint8_t eighths_per;
        V1_pattern_t pattern[256];
    } patterns;

    // Instrument 0 is invalid
    struct {
        uint16_t count;
        uint8_t current;
        V1_instrument_t instrument[256];
    } instruments;

    struct {
        int BPM;
        uint8_t pattern_eighths_per_column[256];
        uint8_t pattern_column_current;
        int note_width;
        int note_offset;
        int loop_point;
    } properties;
} V2_music_t;

V2_music_t music_data;
constexpr int TRACK_COUNT = sizeof(music_data.tracks.track) / sizeof(*music_data.tracks.track);

bool track_is_used[TRACK_COUNT] = {};

uint8_t music_track_count = 0;

#define EIGHTHS_PER_PATTERN_MAX 64
#define PATTERNS_PER_TRACK 128
#define BPM_DEFAULT 80
#define EIGHTHS_PER_PATTERN_DEFAULT 32
static float SPB = 1.f / ((float)BPM_DEFAULT / 60);
static float SP8th = (1.f / ((float)BPM_DEFAULT / 60)) / 8.f;// SPB / 8.f;
static int32_t samples_per_beat = 48000 / (BPM_DEFAULT * 8 / 60);
// Size large enough for every single beat of every pattern of every track to be occupied
sound_t prepared_sounds[TRACK_COUNT][PATTERNS_PER_TRACK][EIGHTHS_PER_PATTERN_MAX];
typedef enum { waveform_sine, waveform_triangle, waveform_saw, waveform_square, waveform_noise } waveform_e;
#define WAVEFORM_COUNT (waveform_noise + 1)
const sound_waveform_e waveform_to_sound_sample[WAVEFORM_COUNT] = {[waveform_sine] = sound_waveform_sine, [waveform_triangle] = sound_waveform_triangle, [waveform_saw] = sound_waveform_saw, [waveform_square] = sound_waveform_pulse, [waveform_noise] = sound_waveform_noise};
#define NOTE_MIN 33

void ExploreFolder(const char *directory) {
    printf ("Entering folder %s\n", directory);
	folder_ChangeDirectory(directory);
	auto result = folder_FindFirstFile("./");
	if (result.is_error) {
	    chdir("../");
		return;
	}
	int curlen = strlen(current_directory);
	assert(snprintf(&current_directory[curlen],
					sizeof(current_directory) - curlen, "%s/",
					directory) == strlen(directory) + 1);
	folder_t folder = result.folder;
	do {
		if (folder.is_folder) {
			if (strcmp(folder.name, "font") == 0) {
				resources_font_t font;
				font_Load(&font, folder.name);
				char codename[2048] = "";
				int len = strlen(current_directory);
				char *s, *d;
				s = current_directory;
				d = codename;
				for (int i = 0; i < len; ++i) {
					if (*s == '/')
					*d = '_';
					else
					*d = *s;
					++d, ++s;
				}
				assert(snprintf(&codename[len], sizeof(codename) - len, "%s",
								folder.name) == strlen(folder.name));
				for (int i = 0; i < BITMAP_FONT_NUM_VISIBLE_CHARS; ++i) {
					fprintf (phil, "const sprite_t %s_bitmap_%d={.w=%d,.h=%d,.p=(uint8_t[]){", codename, i, font.bitmaps[i].w, font.bitmaps[i].h);
					for (int j = 0; j < font.bitmaps[i].w*font.bitmaps[i].h; ++j) {
						fprintf (phil, "%d,", font.pixels[font.bitmaps[i].offset + j]);
					}
					fprintf (phil, "}};\n");
				}
				fprintf (header, "extern const font_t %s;\n", codename);
				fprintf (phil, "const font_t %s={.line_height=%d,.baseline=%d,.space_width=%d,.bitmaps={", codename, font.line_height, font.baseline, font.space_width);
				for (int i = 0; i < BITMAP_FONT_NUM_VISIBLE_CHARS; ++i) {
					fprintf (phil, "&%s_bitmap_%d,", codename, i);
				}
				fprintf (phil, "},.descent={");
				for (int i = 0; i < BITMAP_FONT_NUM_VISIBLE_CHARS; ++i) {
					fprintf (phil, "%d,", font.descent[i]);
				}
				fprintf (phil, "}};\n");
                continue;
            } else if (strcmp (folder.name, "cursor") == 0) {
                char codename[2048] = "";
                int namelen = strlen(current_directory);
                char *s, *d;
                s = current_directory;
                d = codename;
                for (int i = 0; i < namelen; ++i) {
                    if (*s == '/')
                        *d = '_';
                    else
                        *d = *s;
                    ++d, ++s;
                } 
                namelen = strlen(codename);
                assert(snprintf(&codename[namelen], sizeof(codename) - namelen, "%s", folder.name) == strlen(folder.name));
                if (LoadCursor (folder.name, codename)) continue;
                // If didn't successfully treat folder as a cursor, instead pass through and treat it as a normal folder
            }
            ExploreFolder(folder.name);
			continue;
		}
		const char *extension = StringGetFileExtension(folder.name);
		char codename[2048] = "";
		int namelen = strlen(current_directory);
		char *s, *d;
		s = current_directory;
		d = codename;
		for (int i = 0; i < namelen; ++i) {
			if (*s == '/')
				*d = '_';
			else
				*d = *s;
			++d, ++s;
		} 
		namelen = strlen(codename);
		assert(snprintf(&codename[namelen], sizeof(codename) - namelen, "%s", folder.name) == strlen(folder.name));
		d = codename;
		while (*d != '.') ++d;
		*d = 0;
        if (StringCompareCaseInsensitive(folder.name, "palette.bmp") == 0) {
            printf ("Loading palette: %s%s\n", current_directory, folder.name);

            fputs("const uint8_t palette[256][3] = {", phil);

            typedef struct __attribute((__packed__)) {
                struct __attribute((__packed__)) {
                    char magic[2];
                    uint32_t file_size;
                    uint32_t filler;
                    uint32_t image_data_address;
                } file;
                struct __attribute((__packed__)) {
                    uint32_t header_size;
                    int32_t width;
                    int32_t height;
                    uint16_t color_planes;
                    uint16_t bits_per_pixel;
                    uint32_t compression;
                    uint32_t compressed_size;
                    uint32_t pixels_per_m_horizontal;
                    uint32_t pixels_per_m_vertical;
                    uint32_t colors_used;
                    uint32_t important_colors;
                } info;
            } bmp_header_t;
            assert (sizeof (bmp_header_t) == 54);
            bmp_header_t header = {};
            FILE *palettebmp = fopen (folder.name, "rb");
            assert (palettebmp);
            fread (&header.file, sizeof (header.file), 1, palettebmp);
            fread (&header.info.header_size, sizeof (header.info.header_size), 1, palettebmp);
            {
                char buf[header.info.header_size - sizeof (header.info.header_size)];
                fread (&buf, sizeof (buf), 1, palettebmp);
            }

            for (int i = 0; i < 256; ++i) {
                uint8_t bgr[3];
                fread (bgr, sizeof (bgr), 1, palettebmp);
                fprintf (phil, "{%u,%u,%u},", bgr[2], bgr[1], bgr[0]);
                fgetc (palettebmp);
            }
            fclose (palettebmp); 
            fputs ("};\n", phil);
        }
		else if (StringCompareCaseInsensitive(extension, "bmp") == 0) {
			printf("Loading bmp: %s%s\n", current_directory, folder.name);
			resources_sprite_t spr = sprite_LoadBMP(folder.name);
			fprintf (header, "extern const sprite_t %s;\n", codename);
			fprintf(phil, "const sprite_t %s = {.w = %u, .h = %u, .p = (uint8_t[]){",
					codename, spr.w, spr.h);
			for (int i = 0; i < spr.w * spr.h; ++i) {
				fprintf(phil, "%u,", spr.p[i]);
			}
			free(spr.p);
			fprintf(phil, "}};\n");
		}
		else if (StringCompareCaseInsensitive(extension, "ktune") == 0) {
			printf("Loading ktune: %s%s\n", current_directory, folder.name);
			if (LoadMusic (folder.name)) {
                fprintf (header, "extern const sound_music_t %s;\n", codename);
                for (uint8_t track = 0; track < TRACK_COUNT; ++track) {
                    if (!track_is_used[track]) continue;
                    fprintf (phil, "const sound_t %s_sounds_%"PRIu8"[] = {", codename, track);
                    const sound_t *sound = &prepared_sounds[track][0][0];
                    uint64_t nextindex = 0;
                    do { // Loop through every sound until ->next == NULL (last sound of track) or it's an earlier sound, meaning it loops at that point
                        ++nextindex;
                        if (sound->next < sound) {
                            const sound_t *loopsound = &prepared_sounds[track][0][0];
                            nextindex = 0;
                            while (loopsound != sound->next) {
                                loopsound = loopsound->next;
                                ++nextindex;
                            }

                        }
                        fprintf (phil, "{.duration = %"PRIu32", .next = &%s_sounds_%"PRIu8"[%"PRIu64"], .frequency = %"PRIu16", .ADSR = {.peak = %f, .attack = %"PRIu16", .decay = %"PRIu16", .sustain = %f, .release = %"PRIu16"}, .vibrato = {.frequency_range = %"PRIu16", .vibrations_per_hundred_seconds = %"PRIu16"}, .sweep = %"PRId16", .square_duty_cycle = %"PRIi8", .square_duty_cycle_sweep = %"PRIi8", .waveform = %s},\n", sound->duration, codename, track, nextindex, sound->frequency, sound->ADSR.peak, sound->ADSR.attack, sound->ADSR.decay, sound->ADSR.sustain, sound->ADSR.release, sound->vibrato.frequency_range, sound->vibrato.vibrations_per_hundred_seconds, sound->sweep, sound->square_duty_cycle, sound->square_duty_cycle_sweep, waveform_to_string[sound->waveform]);
                    } while (sound->next > sound && (sound = sound->next));
                    fprintf (phil, "};\n");
                }
                fprintf (phil,   "const sound_music_t %s = {%"PRIu8",{", codename, music_track_count);
                for (uint8_t track = 0; track < TRACK_COUNT; ++track) {
                    if (!track_is_used[track]) continue;
                    fprintf (phil, "%s_sounds_%"PRIu8"," , codename, track);
                }
                fprintf (phil, "},};\n");
            }
		}
  	} while (!folder_FindNextFile(&folder).is_error);
	PopDir();
}

void CreateNewFiles () {
    if (phil && ftell(phil) > 0) fclose (phil);
    if (header && ftell(header)) fclose (header);

	phil = fopen("resources.c", "w");
	assert(phil);

	header = fopen ("resources.h", "w");
	assert (header);

	fprintf (header,
R"(#pragma once

#include <stdint.h>

#include "framework.h"

)");

    fputs("#include \"resources.h\"\n", phil);
}

int main(int argc, char **argv) {
    assert (argc > 1);

    --argc;
    ++argv;

	printf("Building resources! I received these arguments:\n");
    for (int i = 0; i < argc; ++i) printf ("%s\n", argv[i]);
    printf ("\n\n");

    const auto base_dir = getcwd (NULL, 0);
    assert (base_dir);

    printf ("Running in base dir [%s]\n", base_dir);

    auto start_dir = base_dir;
    if (strcmp(*argv, "--new") != 0) {
        CreateNewFiles();

        printf ("Creating new files from [%s]\n", start_dir);
    }

    while (argc--) {
        if (strcmp(*argv, "--new") == 0) {
            assert (argc); --argc; ++argv;
            start_dir = *argv;
            printf ("\n\nCreating new files from [%s]\n", start_dir);
            assert (!folder_ChangeDirectory(base_dir).is_error);
            assert (!folder_ChangeDirectory(start_dir).is_error);
            CreateNewFiles ();
        }
        else {
            printf ("Entering directory [%s]\n", *argv);
            assert (!folder_ChangeDirectory(*argv).is_error);
            ExploreFolder ("resources");
        }
        ++argv;
        current_directory[0] = 0;
        assert (!folder_ChangeDirectory(base_dir).is_error);
        if (start_dir != base_dir) assert (!folder_ChangeDirectory(start_dir).is_error);
    }

	fclose(phil);
	printf("Goodbye world!\n");
	return 0;
}





resources_sprite_t sprite_LoadBMP (const char *filename) {
	assert (filename != NULL);

	struct {
		char BM[2];
		uint32_t size, reserved, data_address, header_size;
		int32_t width, height;
		uint16_t color_planes;
		uint16_t bit_depth;
	} __attribute__((__packed__)) header;

	uint32_t pixel_count;

	FILE* file = fopen(filename, "rb");
	assert (file);

	fread(&header, sizeof (header), 1, file);

	pixel_count = header.width * header.height;
	int required_memory = sizeof ((resources_sprite_t){}.p[0]) * pixel_count;

	assert (header.bit_depth == 8);

	assert (fseek(file, header.data_address, SEEK_SET) == 0);

	uint8_t *pixels = malloc(required_memory);
	assert (pixels);

	int padding_per_line = 4 - header.width % 4;
	if (padding_per_line == 4) padding_per_line = 0;
	for (int y = 0; y < header.height; ++y) {
		uint32_t pixels_read = fread(&pixels[y * header.width], 1, header.width, file);
		assert (pixels_read == header.width);
		assert (fseek (file, padding_per_line, SEEK_CUR) == 0);
	}

	fclose (file);

	return (resources_sprite_t){.w = header.width, .h = header.height, .p = pixels};
}






// Pass in the folder which contains the font files: font.bmp/tga and properties.txt
// Returns false on failure and true on success
bool font_Load (resources_font_t *font, char *directory) {
    assert (directory);
    bool return_value = false;

    int directory_length = strlen (directory);
    assert (directory_length < 512);

    char last_char = directory[directory_length-1]; // Go back from the NULL
    bool ending_slash = false;
    if (last_char == '\'' || last_char == '/') {
        ending_slash = true;
    }
    // Copy the directory, adding a slash at the end if it's missing.
    char directory_fixed[513];
    sprintf (directory_fixed, "%s%c", directory, ending_slash ? '\0' : '/');

    char filename[600];
    
    sprintf (filename, "%sfont.bmp", directory_fixed);
    resources_sprite_t bitmap = sprite_LoadBMP (filename);

    // Convert bitmap to individual character bitmaps
    int width, height;
    width = bitmap.width / 10;
    height = bitmap.height / 10;
    font->space_width = width/2; font->line_height = height + 4; // Just defaults in case they aren't set inside properties.txt
    font->pixels = (uint8_t*) malloc (width * height * BITMAP_FONT_NUM_VISIBLE_CHARS);
    assert (font->pixels);
    
    int x, y;
    int cumulative_pixels = 0;
    for (int i = 0; i < BITMAP_FONT_NUM_VISIBLE_CHARS; ++i) {
        y = i / 10;
        x = i % 10;
        struct {
            int left, right, bottom, top, width, height;
        } source = {};

        for (int xx = 0; xx < width; ++xx) {
            for (int yy = 0; yy < height; ++yy) {
                if (bitmap.p[(yy + y * height) * bitmap.width + x * width + xx] != 0) {
                    source.left = xx;
                    yy = height;
                    xx = width;
                }
            }
        }
        for (int xx = width - 1; xx >= 0; --xx) {
            for (int yy = 0; yy < height; ++yy) {
                if (bitmap.p[(yy + y * height) * bitmap.width + x * width + xx] != 0) {
                    source.right = xx;
                    yy = height;
                    xx = -1;
                }
            }
        }
        for (int yy = 0; yy < height; ++yy) {
            for (int xx = 0; xx < width; ++xx) {
                if (bitmap.p[(yy + y * height) * bitmap.width + x * width + xx] != 0) {
                    source.bottom = yy;
                    xx = width;
                    yy = height;
                }
            }
        }
        for (int yy = height-1; yy >= 0; --yy) {
            for (int xx = 0; xx < width; ++xx) {
                if (bitmap.p[(yy + y * height) * bitmap.width + x * width + xx] != 0) {
                    source.top = yy;
                    xx = width;
                    yy = -1;
                }
            }
        }

        source.width = source.right - source.left + 1;
        source.height = source.top - source.bottom + 1;

        font->bitmaps[i].w = source.width;
        font->bitmaps[i].h = source.height;
        font->descent[i] = 0;
        
        font->bitmaps[i].offset = cumulative_pixels;
        for (int sy = 0; sy < source.height; ++sy) {
            memcpy (&font->pixels[font->bitmaps[i].offset + sy * source.width], &bitmap.p[(sy + source.bottom + y * height) * bitmap.width + x * width + source.left], font->bitmaps[i].w);
        }
        cumulative_pixels += font->bitmaps[i].w * font->bitmaps[i].h;
    }

    free (bitmap.p);

    font->pixels = (uint8_t*)realloc (font->pixels, cumulative_pixels);

    sprintf (filename, "%sproperties.txt", directory_fixed);

    char *contents;
    contents = ReadEntireFile (filename, NULL);
    if (contents == NULL) {
        PRINT_ERROR ("Failed to read file: %s", filename);
        assert (false);
    }

    char *c = contents;
    int i;
    char property;
    int value;
    font->baseline = 0;
    while (*c != '\0') {
        i = *c;
        if (i >= BITMAP_FONT_FIRST_VISIBLE_CHAR && i <= BITMAP_FONT_LAST_VISIBLE_CHAR) {
            i -= BITMAP_FONT_FIRST_VISIBLE_CHAR;
            ++c; // Skip the letter to get to the space before first property
            // Now we're at the list of properties for this letter [i]
            do {
                ++c;
                property = *c;
                value = atoi (c+1);
                switch (property) {
                    case 'd': font->descent[i] = value; break;
                    default: {
                        PRINT_ERROR ("Reading font \'%s\' encountered invalid property \'%c\' in line of character \'%c\'", directory, property, (char)(i + BITMAP_FONT_FIRST_VISIBLE_CHAR));
                        assert (false);
                    }
                }
                c = StringSkipNonWhiteSpace (c); // Skip the property value
            } while (*c == ' '); // If it's a space, there's another property
            c = StringSkipWhiteSpace (c); // Otherwise we need to get to the next line and the next letter
        }
        else if (*c == ' ') { // Font properties
            do {
                ++c;
                property = *c;
                value = atoi (c+1);
                switch (property) {
                    case 'w': font->space_width = value; break;
                    case 'h': font->line_height = value; break;
                    case 'b': font->baseline    = value; break;
                    default: {
                        PRINT_ERROR ("Reading font \'%s\' encountered invalid property \'%c\' in line of font properties (line which starts with space).", directory, property);
                        assert (false);
                    }
                }
                c = StringSkipNonWhiteSpace (c); // Skip the property value
            } while (*c == ' '); // If it's a space, there's another property
            c = StringSkipWhiteSpace (c); // Otherwise we need to get to the next line and the next letter
        }
        else {
            PRINT_ERROR ("Invalid font file. Format for each line should always be: \"<ascii character> p<property number>\". Error encountered here: %s", c);
            assert (false);
        }
    }
    if (font->baseline == 0) font->baseline = font->line_height;

    return_value = true;

    free (contents);
    return return_value;
}

bool LoadFileV2 (FILE *file) {
    if (fread (&music_data, sizeof (music_data), 1, file) != 1) return false;
    return true;
}

static void BPM_Change (int amount) {
	if (amount > 0) music_data.properties.BPM = MIN (200, music_data.properties.BPM+1);
	else if (amount < 0) music_data.properties.BPM = MAX (20, music_data.properties.BPM-1);
	// else if amount == 0 do nothing
	SPB = 1.f / ((float)music_data.properties.BPM / 60);
	SP8th = (1.f / ((float)music_data.properties.BPM / 60)) / 8.f;// SPB / 8.f;
    samples_per_beat = 48000 / (music_data.properties.BPM * 8 / 60);
}

sound_t SoundGenerate_ (SoundFXPlay_args args);
#define SoundGenerate(...) SoundGenerate_((SoundFXPlay_args){SoundFXPlay_args_default, __VA_ARGS__})
#define SAMPLING_RATE 48000
ADSR_t ADSRf_to_ADSR (ADSRf_t in) { return (ADSR_t){.peak = in.peak, .attack = in.attack * SAMPLING_RATE, .decay = in.decay * SAMPLING_RATE, .sustain = in.sustain, .release = in.release * SAMPLING_RATE}; }

sound_t SoundGenerate_(SoundFXPlay_args args) {
    assert (args.waveform);
    if (args.waveform == sound_waveform_none) return (sound_t){};
    args.ADSR.peak *= args.volume;
    args.ADSR.sustain *= args.volume;
    uint32_t duration = args.duration_samples;
    if (duration == 0) duration = args.duration_seconds * SAMPLING_RATE;
    sound_t sound = {
        .waveform = args.waveform,
        .duration = duration,
        .next = args.next,
        .ADSR = ADSRf_to_ADSR(args.ADSR),
        .frequency = args.frequency,
        .sweep = args.sweep,
        .vibrato = args.vibrato,
        .square_duty_cycle = args.square_duty_cycle,
        .square_duty_cycle_sweep = args.square_duty_cycle_sweep,
    };
    // if (sound.waveform == sound_waveform_pulse) {
    //     if (sound.vibrato.frequency_range < sound.frequency * .005f) sound.vibrato.frequency_range = sound.frequency * .005f;
    //     if (sound.vibrato.vibrations_per_hundred_seconds < sound.frequency/20) sound.vibrato.vibrations_per_hundred_seconds = sound.frequency/20;
    // }
    int duration_ADSR = sound.ADSR.attack + sound.ADSR.decay + sound.ADSR.release;
    if (sound.duration < duration_ADSR) { // There isn't enough time for the full envelope, even with 0 sustain. Get ratio of envelope and apply to the reduced duration
        float total = args.ADSR.attack + args.ADSR.decay + args.ADSR.release;
        sound.ADSR.attack = (args.ADSR.attack / total) * sound.duration;
        sound.ADSR.release = (args.ADSR.release / total) * sound.duration;
        sound.ADSR.decay = sound.duration - sound.ADSR.attack - sound.ADSR.release;
    }
    return sound;
}

static void PrepareTrackPattern (uint8_t track, uint8_t pattern) {
    track_is_used[track] = true;
    auto t = &music_data.tracks.track[track];
    auto pi = t->patterns[pattern];
    if (pi == 0) return;
    auto p = &music_data.patterns.pattern[pi];
    auto ti = &music_data.instruments.instrument[t->instrument];
    uint8_t pattern_width = music_data.properties.pattern_eighths_per_column[pattern] ? music_data.properties.pattern_eighths_per_column[pattern] : music_data.patterns.eighths_per;
    for (int beat = 0; beat < pattern_width; ++beat) {
        auto b = &p->beats[beat];
        sound_t *nextsound;
        if (pattern < PATTERNS_PER_TRACK-1) nextsound = &prepared_sounds[track][pattern+1][0];
        else nextsound = NULL;
        if (b->eighths) {
            auto i = ti;
            if (b->instrument_override)
                i = &music_data.instruments.instrument[b->instrument_override];
            ADSRf_t ADSR = i->ADSR;
            if (b->lead_in) ADSR.attack = ADSR.decay = 0;
            if (b->lead_out) ADSR.release = 0;
            uint8_t beat_width = b->eighths;
            if (beat + b->eighths > pattern_width)
                beat_width = pattern_width - beat;
            if (beat + b->eighths < pattern_width) nextsound = &prepared_sounds[track][pattern][beat + beat_width];
            prepared_sounds[track][pattern][beat] = SoundGenerate (.waveform = waveform_to_sound_sample[i->waveform], .frequency = midi_note_frequency[b->note + NOTE_MIN], .duration_samples = beat_width * samples_per_beat, .volume = t->volume / 255.f, .ADSR = ADSR, .sweep = i->sweep, .vibrato = i->vibrato, .next = nextsound, .square_duty_cycle = i->square_duty_cycle, .square_duty_cycle_sweep = i->square_duty_cycle_sweep);
        }
        else {
            if (beat < pattern_width-1) nextsound = &prepared_sounds[track][pattern][beat+1];
            prepared_sounds[track][pattern][beat] = SoundGenerate (.waveform = sound_waveform_silence, .duration_samples = samples_per_beat, .next = nextsound);
        }
    }
}

static void PrepareTrack (uint8_t track) {
    track_is_used[track] = false;
    auto t = &music_data.tracks.track[track];
    for (int i = 0; i < PATTERNS_PER_TRACK; ++i) {
        int p = t->patterns[i];
        if (p) PrepareTrackPattern (track, i);
        else {
            int pattern_width = music_data.properties.pattern_eighths_per_column[i] ? music_data.properties.pattern_eighths_per_column[i] : music_data.patterns.eighths_per;
            for (int j = 0; j < pattern_width-1; ++j) {
                prepared_sounds[track][i][j] = SoundGenerate (.waveform = sound_waveform_silence, .duration_samples = samples_per_beat, .next = &prepared_sounds[track][i][j+1]);
            }
            if (track < PATTERNS_PER_TRACK) prepared_sounds[track][i][pattern_width-1] = SoundGenerate (.waveform = sound_waveform_silence, .duration_samples = samples_per_beat, .next = &prepared_sounds[track][i+1][0]);
            else prepared_sounds[track][i][pattern_width-1] = SoundGenerate (.waveform = sound_waveform_silence, .duration_samples = samples_per_beat);
        }
    }
}

static bool LoadMusic (const char *filename) {
    bool success = false;
    defer { if (!success) { printf ("Failed to load file [%s]\n", filename); } }
    FILE *file = fopen (filename, "rb");
    assert (file); if (!file) { LOG ("Failed to open file [%s]", filename); return false; }
    defer { fclose (file) }
    struct {
        char magic[14];
        uint32_t version;
    } file_identifier;
    if (fread (&file_identifier, sizeof (file_identifier), 1, file) != 1) return false;
    if (strcmp (file_identifier.magic, "Kero Chiptune") != 0) return false;
    bool load_result = false;
    switch (file_identifier.version) {
        case 2: load_result = LoadFileV2 (file); break;
        default: {
            printf ("Unsupported file version\n");
        } break;
    }
    if (!load_result) return false;

    BPM_Change(0); // Recalculate stuff. Does not actually set BPM to 0 when called with 0

    music_track_count = 0;
    
    for (int i = 0; i < TRACK_COUNT; ++i) {
        PrepareTrack (i);
        if (track_is_used[i]) ++music_track_count;
    }
    
    int final_column = 0;
    for (int track = 0; track < TRACK_COUNT; ++track) {
        for (int column = PATTERNS_PER_TRACK-1; column >= 0; --column) {
            if (music_data.tracks.track[track].patterns[column]) {
                final_column = MAX (final_column, column);
            }
        }
    }
    int final_column_width = music_data.properties.pattern_eighths_per_column[final_column] ? music_data.properties.pattern_eighths_per_column[final_column] : music_data.patterns.eighths_per;

    // Loop
    for (int track = 0; track < TRACK_COUNT; ++track) {
        if (!track_is_used[track]) continue;
        prepared_sounds[track][final_column][final_column_width-1].next = &prepared_sounds[track][music_data.properties.loop_point][0];
        auto pi = music_data.tracks.track[track].patterns[final_column];
        if (pi) {
            for (int b = final_column_width-1; b >= 0; --b) {
                auto w = music_data.patterns.pattern[pi].beats[b].eighths;
                if (w > 0) {
                    if (b + w == final_column_width)
                        prepared_sounds[track][final_column][b].next = &prepared_sounds[track][music_data.properties.loop_point][0];
                    break;
                }
            }
        }
    }
    
    for (int track = 0; track < TRACK_COUNT; ++track) {
        if (!track_is_used[track]) continue;
    }

    success = true;

    return true;
}

// Returns an allocated char array of the entire file contents.
// YOU MUST DEALLOCATE THE POINTER!
// Prints errors and returns NULL on failure.
char *ReadEntireFile (char *filename, int *return_file_length) {
    char *buffer;
    FILE *file = fopen (filename, "rb");
    if (!file) {
        PRINT_ERROR ("Failed to open file: %s. Maybe it doesn't exist?", filename);
        return NULL;
    }
    fseek (file, 0, SEEK_END);
    int file_length = ftell (file);
    buffer = malloc (file_length+1);
    if (!buffer) {
        PRINT_ERROR ("Failed to allocate memory for file buffer when reading file \"%s\" of length %d", filename, file_length);
        fclose (file);
        return NULL;
    }
    rewind (file);
    int bytes_read = fread (buffer, 1, file_length, file);
    fclose (file);
    if (bytes_read != file_length) {
        PRINT_ERROR ("Failed to read file. Expected to read %d bytes. Read %d bytes.", file_length, bytes_read);
        free (buffer);
        return NULL;
    }
    buffer[file_length] = 0;
    if (return_file_length) *return_file_length = file_length;
    return buffer;
}

bool LoadCursor (const char *const folder, const char *const codename) {
    printf ("Loading cursor [%s]\n", codename);
    auto result = folder_ChangeDirectory(folder);
    if (result.is_error) return false;
    defer { folder_ChangeDirectory (".."); }

    struct {
        int x, y;
    } cursor = {};
    

    int len = 0;
    auto props = ReadEntireFile("properties.txt", &len);
    if (props == NULL) {
        printf ("Cursor properties file not found\n");
        return false;
    }
    defer { free (props); }

    {
        FILE *file_sprite = fopen ("sprite.bmp", "rb");
        if (file_sprite == NULL) {
            printf ("sprite.bmp file not found\n");
            return false;
        }
        fclose (file_sprite);
    }

    auto sprite = sprite_LoadBMP("sprite.bmp");
    if (sprite.w == 0 || sprite.h == 0) {
        printf ("Sprite file invalid\n");
        return false;
    }

    const char *p = props;
    while (p < props + len) {
        char key;
        int value;
        if (sscanf (p, "%c %d", &key, &value) != 2) break;
        switch (key) {
            case 'x': cursor.x = value; break;
            case 'y': cursor.y = value; break;
            default: printf ("Invalid key [%c]\n", key); return false;
        }
        while (*p != '\n' && p < props + len) ++p;
        ++p;
    }

    fprintf (header, "extern const cursor_t %s;\n", codename);
    fprintf (phil, "const cursor_t %s = {.offset = {.x = %d, .y = %d}, .sprite = &(sprite_t){.w = %d, .h = %d, .p = (uint8_t[]){", codename, cursor.x, cursor.y, sprite.w, sprite.h);
    for (int i = 0; i < sprite.w * sprite.h; ++i) {
        fprintf (phil, "%d,", sprite.p[i]);
    }
    fprintf (phil, "}}};\n");

    return true;
}
