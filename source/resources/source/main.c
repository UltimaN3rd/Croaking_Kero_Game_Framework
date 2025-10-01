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

uint8_t palette[256][3];

FILE *phil, *header;

char current_directory[2048] = "";

typedef struct {
	union { int width,  w; };
	union { int height, h; };
	uint8_t *p;
} sprite_t;

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
} font_t;

void LoadSound (const char *filename, const char *codename);
sprite_t sprite_LoadBMP (const char *filename);
bool font_Load (font_t *font, char *directory);
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

void ExploreFolder(const char *directory) {
	folder_ChangeDirectory(directory);
	auto result = folder_FindFirstFile("./");
	if (result.is_error) {
		PopDir();
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
				font_t font;
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
				// fprintf (phil, "const uint8_t %s_pixels[]={", codename);
				// for (int i = 0; i < BITMAP_FONT_NUM_VISIBLE_CHARS; ++i) {
				// 	for (int j = 0; j < font.bitmaps[i].w*font.bitmaps[i].h; ++j) {
				// 		fprintf (phil, "%d,", font.pixels[font.bitmaps[i].offset + j]);
				// 	}
				// }
				for (int i = 0; i < BITMAP_FONT_NUM_VISIBLE_CHARS; ++i) {
					fprintf (phil, "const sprite_t %s_bitmap_%d={.w=%d,.h=%d,.p={", codename, i, font.bitmaps[i].w, font.bitmaps[i].h);
					for (int j = 0; j < font.bitmaps[i].w*font.bitmaps[i].h; ++j) {
						fprintf (phil, "%d,", font.pixels[font.bitmaps[i].offset + j]);
					}
					fprintf (phil, "}};\n");
				}
				fprintf (header, "extern const font_t %s;\n", codename);
				fprintf (phil, "const font_t %s={.line_height=%d,.baseline=%d,.space_width=%d,.bitmaps={", codename, font.line_height, font.baseline, font.space_width);
				for (int i = 0; i < BITMAP_FONT_NUM_VISIBLE_CHARS; ++i) {
					fprintf (phil, "&%s_bitmap_%d,", codename, i);
					/*fprintf (phil, "&(sprite_t){.w=%d,.h=%d,.p={", font.bitmaps[i].w, font.bitmaps[i].h);
					for (int j = 0; j < font.bitmaps[i].w*font.bitmaps[i].h; ++j) {
						fprintf (phil, "%d,", font.pixels[font.bitmaps[i].offset + j]);
					}
					fprintf (phil, "}},");*/
				}
				fprintf (phil, "},.descent={");
				for (int i = 0; i < BITMAP_FONT_NUM_VISIBLE_CHARS; ++i) {
					fprintf (phil, "%d,", font.descent[i]);
				}
				fprintf (phil, "}};\n");
			} else {
				ExploreFolder(folder.name);
			}
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
		if (StringCompareCaseInsensitive(extension, "bmp") == 0) {
			printf("Loading bmp: %s%s\n", current_directory, folder.name);
			sprite_t spr = sprite_LoadBMP(folder.name);
			fprintf (header, "extern const sprite_t %s;\n", codename);
			fprintf(phil, "const sprite_t %s = {.w = %u, .h = %u, .p = {",
					codename, spr.w, spr.h);
			for (int i = 0; i < spr.w * spr.h; ++i) {
				fprintf(phil, "%u,", spr.p[i]);
			}
			free(spr.p);
			fprintf(phil, "}};\n");
		}
		else if (StringCompareCaseInsensitive(extension, "ksound") == 0) {
			printf("Loading ksound: %s%s\n", current_directory, folder.name);
			LoadSound (folder.name, codename);
		}
  	} while (!folder_FindNextFile(&folder).is_error);
	PopDir();
}

int main(int argc, char **argv) {
	printf("Hello world!\n");

	phil = fopen("resources.c", "w");
	assert(phil);

	header = fopen ("resources.h", "w");
	assert (header);

	fprintf (header,
R"(#pragma once

#include <stdint.h>

typedef struct {
	uint16_t w, h;
	uint8_t p[];
} sprite_t;

#include "sound.h"
#include "samples.h"

#define BITMAP_FONT_FIRST_VISIBLE_CHAR 33
#define BITMAP_FONT_LAST_VISIBLE_CHAR 126
#define BITMAP_FONT_NUM_VISIBLE_CHARS (BITMAP_FONT_LAST_VISIBLE_CHAR - BITMAP_FONT_FIRST_VISIBLE_CHAR + 1)

typedef struct {
    int8_t line_height, baseline;
    uint8_t space_width;
    const sprite_t *bitmaps[BITMAP_FONT_NUM_VISIBLE_CHARS];
    int8_t descent[BITMAP_FONT_NUM_VISIBLE_CHARS];
} font_t;

extern const uint8_t palette[256][3];
)");

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
    FILE *palettebmp = fopen ("palette.bmp", "rb");
    assert (palettebmp);
    fread (&header.file, sizeof (header.file), 1, palettebmp);
    fread (&header.info.header_size, sizeof (header.info.header_size), 1, palettebmp);
    {
        char buf[header.info.header_size - sizeof (header.info.header_size)];
        fread (&buf, sizeof (buf), 1, palettebmp);
    }

	fputs(R"(#include "resources.h"

const uint8_t palette[256][3] = {)", phil);

	for (int i = 0; i < 256; ++i) {
        uint8_t bgr[3];
		fread (bgr, sizeof (bgr), 1, palettebmp);
        fprintf (phil, "{%u,%u,%u},", bgr[2], bgr[1], bgr[0]);
		fgetc (palettebmp);
	}
	fclose (palettebmp); 
    fputs ("};\n", phil);

	ExploreFolder("resources");

	fclose(phil);
	printf("Goodbye world!\n");
	return 0;
}





sprite_t sprite_LoadBMP (const char *filename) {
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
	int required_memory = sizeof ((sprite_t){}.p[0]) * pixel_count;

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

	return (sprite_t){.w = header.width, .h = header.height, .p = pixels};
}






// Pass in the folder which contains the font files: font.bmp/tga and properties.txt
// Returns false on failure and true on success
bool font_Load (font_t *font, char *directory) {
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
    sprite_t bitmap = sprite_LoadBMP (filename);

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






typedef struct {
	uint32_t count;
	[[gnu::counted_by(count)]] int16_t samples[];
} sound_sample_t;

typedef struct {
    float peak;
    uint16_t attack;
    uint16_t decay;
    float sustain;
    uint16_t release;
} ADSR_t;

typedef struct {
    float peak;
    float attack;
    float decay;
    float sustain;
    float release;
} ADSRf_t;

typedef struct SoundPlay_args SoundPlay_args;
void *Sound(void*);
struct SoundPlay_args {
    const sound_sample_t *sample;
    uint16_t frequency, frequency_end;
    float duration;
    float volume;
    ADSRf_t ADSR;
    SoundPlay_args *next;
};

typedef struct sound_t {
    uint32_t t;
    uint32_t duration;
    const struct sound_t *next;
    float d;
    int16_t frequency_begin, frequency_end;
    uint8_t ADSR_state;
    ADSR_t ADSR;
    const sound_sample_t *sample;
} sound_t;

#define sampling_rate 48000
#define period_size (sampling_rate / 200)

#define SoundPlay_args_default .frequency = 500, .frequency_end = UINT16_MAX, .duration = 0.25, .volume = 1.0, .ADSR = {.peak = 1, .sustain = .8, .attack = .01, .decay = .005, .release = .01}

sound_t SoundGenerate_ (SoundPlay_args args);
#define SoundGenerate(...) SoundGenerate_((SoundPlay_args){SoundPlay_args_default, __VA_ARGS__})

ADSR_t ADSRf_to_ADSR (ADSRf_t in) { return (ADSR_t){.peak = in.peak, .attack = in.attack * sampling_rate, .decay = in.decay * sampling_rate, .sustain = in.sustain, .release = in.release * sampling_rate}; }

sound_t SoundGenerate_(SoundPlay_args args) {
    if (args.frequency_end == UINT16_MAX) {
        args.frequency_end = args.frequency;
    }
    sound_t sound = {
        .sample = args.sample,
        .duration = args.duration * sampling_rate,
        .next = NULL,
        .ADSR = ADSRf_to_ADSR(args.ADSR),
        .frequency_begin = args.frequency,
        .frequency_end = args.frequency_end,
    };
    int duration = sound.ADSR.attack + sound.ADSR.decay + sound.ADSR.release;
    if (sound.duration < duration) { // There isn't enough time for the full envelope, even with 0 sustain. Get ratio of envelope and apply to the reduced duration
        float total = args.ADSR.attack + args.ADSR.decay + args.ADSR.release;
        sound.ADSR.attack = (args.ADSR.attack / total) * sound.duration;
        sound.ADSR.decay = (args.ADSR.decay / total) * sound.duration;
        sound.ADSR.release = sound.duration - sound.ADSR.attack - sound.ADSR.decay;
    }
    return sound;
}

typedef enum { instrument_sine, instrument_triangle, instrument_saw, instrument_square, instrument_noise } V03_instrument_e;
#define V03_INSTRUMENT_COUNT (instrument_noise + 1)

const char *const V03_instrument_to_sound_sample_string[V03_INSTRUMENT_COUNT] = {[instrument_sine] = "sound_sample_sine", [instrument_triangle] = "sound_sample_triangle", [instrument_saw] = "sound_sample_saw", [instrument_square] = "sound_sample_square", [instrument_noise] = "sound_sample_noise"};

#define MIDI_NOTE_COUNT 129

const struct {
    const float note_frequency[MIDI_NOTE_COUNT];
} midi = {
    .note_frequency = {8.18,8.66,9.18,9.72,10.30,10.91,11.56,12.25,12.98,13.75,14.57,15.43,16.35,17.32,18.35,19.45,20.60,21.83,23.12,24.50,25.96,27.50,29.14,30.87,32.70,34.65,36.71,38.89,41.20,43.65,46.25,49.00,50.91,55.00,58.27,61.74,65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,130.81,138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,261.63,277.18,293.66,311.13,329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,523.25,554.37,587.33,622.25,659.26,698.46,739.99,783.99,830.61,880.00,932.33,987.77,1046.50,1108.73,1174.66,1244.51,1318.51,1396.90,1479.98,1567.98,1661.22,1760.00,1864.66,1975.53,2093.00,2217.46,2349.32,2489.02,2637.02,2793.83,2959.96,3135.96,3322.44,3520.00,3729.30,3951.07,4186.01,4434.92,4698.64,4978.03,5274.04,5587.65,5919.91,6271.93,6644.88,7050.00,7458.62,7902.13,8372.02,8869.84,9397.27,9956.06,10548.08,11175.30,11839.82,12543.85,13289.75}
};

bool LoadSoundV03 (FILE *file, const char *soundname) {
    int BPM, note_width, note_offset;
    constexpr int SOUND_CHANNELS = 10;
    constexpr int CHANNEL_NAME_MAX_LENGTH = 16;
    constexpr int MAX_SOUNDS_PER_CHANNEL = 255;
    struct {
        char name[CHANNEL_NAME_MAX_LENGTH+1];
        union {
            struct { uint8_t b, g, r, a; };
            uint32_t u32;
        } color;
        struct {
            V03_instrument_e waveform;
            ADSRf_t envelope;
        } instrument;
        [[gnu::packed]] struct {
            uint8_t note, length;
            uint16_t beat; // beat is when this sound occurs, in 1/8ths of a beat
        } sequence[MAX_SOUNDS_PER_CHANNEL];
    } channels[SOUND_CHANNELS] = {};
    int channel_sequence_length[SOUND_CHANNELS] = {};
	fread (&BPM, sizeof (BPM), 1, file);
	fread (&note_width, sizeof (note_width), 1, file);
	fread (&note_offset, sizeof (note_offset), 1, file);
	for (int c = 0; c < SOUND_CHANNELS; ++c) {
		fread (&channels[c].color, sizeof (channels[c].color), 1, file);
		fread (&channels[c].instrument, sizeof (channels[c].instrument), 1, file);
		fread (&channels[c].name, sizeof (channels[c].name), 1, file);
		fread (&channel_sequence_length[c], sizeof (channel_sequence_length[c]), 1, file);
		fread (channels[c].sequence, sizeof (channels[c].sequence[0]), channel_sequence_length[c], file);
	}

    float SP8th = (1.f / ((float)BPM / 60)) / 8.f;

    int final_beat = 0;
    for (int c = 0; c < SOUND_CHANNELS; ++c) {
        int beat = 0;
        if (channel_sequence_length[c]) {
            beat = channels[c].sequence[channel_sequence_length[c]-1].beat + channels[c].sequence[channel_sequence_length[c]-1].length;
            if (beat > final_beat) final_beat = beat;
        }
    }

    final_beat += 8;
    int active_channels = 0;
    
    fprintf (header, "extern const sound_music_t %s;\n", soundname);

    int active_channel_indices[SOUND_CHANNELS] = {};
    for (int c = 0; c < SOUND_CHANNELS; ++c) {
        if (channel_sequence_length[c]) {
            active_channel_indices[active_channels++] = c;
            fprintf (phil, "const sound_t %s_%d[] = {", soundname, c);
            int sound_index = 0;
            int last_note_end_beat = 0;
            for (int i = 0; i < channel_sequence_length[c]; ++i) {
                if (last_note_end_beat < channels[c].sequence[i].beat) { // Add silence between last beat and new one
                    auto sound = SoundGenerate (.duration = (channels[c].sequence[i].beat - last_note_end_beat) * SP8th);
                    fprintf (phil, "{.sample=&sound_sample_silence,.duration=%u,.next=&%s_%d[%d]}, ", sound.duration, soundname, c, sound_index+1);
                    ++sound_index;
                }
                auto sound = SoundGenerate (.frequency = midi.note_frequency[channels[c].sequence[i].note], .duration = SP8th * channels[c].sequence[i].length, .ADSR = channels[c].instrument.envelope);
                fprintf (phil, "{.sample=&%s,.duration=%u,.frequency_begin=%u,.frequency_end=%u,.ADSR={.peak=%f,.attack=%u,.decay=%u,.sustain=%f,.release=%u},.next=", V03_instrument_to_sound_sample_string[channels[c].instrument.waveform], sound.duration, sound.frequency_begin, sound.frequency_end, sound.ADSR.peak, sound.ADSR.attack, sound.ADSR.decay, sound.ADSR.sustain, sound.ADSR.release);
                if (i < channel_sequence_length[c]-1) {
                    fprintf (phil, "&%s_%d[%d]}, ", soundname, c, sound_index+1);
                }
                last_note_end_beat = channels[c].sequence[i].beat + channels[c].sequence[i].length;
                ++sound_index;
            }
            if (last_note_end_beat < final_beat) {
                auto sound = SoundGenerate (.duration = (final_beat - last_note_end_beat) * SP8th);
                fprintf (phil, "&%s_%d[%d]}, {.sample=&sound_sample_silence,.duration=%u,.next=%s_%d} };\n", soundname, c, sound_index, sound.duration, soundname, c);
            }
            else {
                fprintf (phil, "%s_%d} };\n", soundname, c);
            }
        }
    }

    assert (active_channels);

    fprintf (phil, "const sound_music_t %s = {.count = %d, .sounds = {", soundname, active_channels);
    for (int i = 0; i < active_channels; ++i) {
        fprintf (phil, "%s_%d,", soundname, active_channel_indices[i]);
    }
    fprintf (phil, "}};\n");

    return true;
}

void LoadSound (const char *filename, const char *codename) {
    FILE *file = fopen (filename, "rb");
    assert (file);

	bool success = false;
	char buf[8] = {};
	fread (buf, strlen ("KSOUND"), 1, file);
	if (strcmp(buf, "KSND") == 0) {
		if (buf[4] == 0) {
			success = true;
			switch (buf[5]) {
				case 3: LoadSoundV03 (file, codename); break;
				default: success = false; break;
			}
		}
	}
	if (!success) {
		printf ("Invalid file. Does not start with a valid magic value");
	}
    fclose (file);

    // {
    //     char buf[8] = {};
    //     fread (buf, strlen ("KSOUND"), 1, file);
    //     assert (StringsAreTheSame(buf, "KSOUND") && strlen (buf) == strlen ("KSOUND"));
    // }
    // int channel_count = fgetc (file);
    // assert (channel_count > 0);
    // int digitarr[16];
    // for (int i = 0; i < channel_count; ++i) {
    //     int sound_count = fgetc (file);
    //     assert (sound_count > 0);
    //     int digits = 0;
    //     for (int div = sound_count; div > 0; div /= 10) ++digits;
    //     digitarr[i] = digits;
    //     for (int j = 0; j < sound_count; ++j) {
    //         if (j > 0) {
    //             fprintf (phil, ",.next=%s%1d%*d};", codename, i, digits, j);
    //         }
    //         fprintf (header, "extern const sound_t %s%1d%*d;\n", codename, i, digits, j);
    //         SoundPlay_args soundargs;
    //         fread (&soundargs, sizeof (soundargs), 1, file);
    //         sound_t sound = SoundGenerate (soundargs);
    //         fprintf(phil, "const sound_t %s%1d%*d={.instrument=%s,.volume=%f,.fadein=%u,.fadeout=%u,.duration=%u", codename, i, digits, j, instrument_names[sound.instrument], sound.volume, sound.fadein, sound.fadeout, sound.duration);
    //         switch (sound.instrument) {
    //             case instrument_sine:
    //                 fprintf (phil, ",.sine={.frequency_begin=%u,.frequency_end=%u}", sound.sine.frequency_begin, sound.sine.frequency_end);
    //                 break;
    //             case instrument_triangle:
    //                 fprintf (phil, ",.triangle={.period_begin=%u,.period_end=%u}", sound.triangle.period_begin, sound.triangle.period_end);
    //                 break;
    //             case instrument_square:
    //                 fprintf (phil, ",.square={.period_begin=%u,.period_end=%u,.pulse_width_begin=%u,.pulse_width_end=%u}", sound.square.period_begin, sound.square.period_end, sound.square.pulse_width_begin, sound.square.pulse_width_end);
    //                 break;
    //             case instrument_noise:
    //                 fprintf (phil, ",.noise={.hold_time_begin=%u,.hold_time_end=%u}", sound.noise.hold_time_begin, sound.noise.hold_time_end);
    //                 break;
    //             case instrument_saw:
    //                 fprintf (phil, ",.saw={.period_begin=%u,.period_end=%u}", sound.saw.period_begin, sound.saw.period_end);
    //                 break;
    //             case instrument_silence:
    //             case instrument_none:
    //             case instrument_preparing:
    //                 break;
    //         }
    //     }
    //     fprintf (phil, "};\n");
    // }
    // fprintf (header, "extern const sound_group_t %s_group;", codename);
    // fprintf (phil, "const sound_group_t %s_group={.count=%d,.sounds={", codename, channel_count);
    // for (int i = 0; i < channel_count; ++i)
    //     fprintf (phil, "&%s%1d%*d,", codename, i, digitarr[i], 0);
    // fprintf (phil, "}};\n");

    // fclose (file);
}






// sound_t* LoadWAV (const char *filename) {
// 	FILE *file;

//     [[gnu::packed]] struct {
//         char magic_riff[4];
//         int32_t filesize;
//         char magic_wavefmt_[8];
//         int32_t format_length;		// 16
//         int16_t format_type;		// 1 = PCM
//         int16_t num_channels;		// 1
//         int32_t sample_rate;		// 44100
//         int32_t bytes_per_second;	// sample_rate * num_channels * bits_per_sample / 8
//         int16_t block_align;		// num_channels * bits_per_sample / 8
//         int16_t bits_per_sample;	// 16
//         char magic_data[4];
//         int32_t data_size;
//     } wav_header;

// 	file = fopen(filename, "rb");
// 	assert (file);

//     fread (&wav_header, sizeof (wav_header), 1, file);

// 	assert (!(wav_header.magic_riff[0] != 'R' || wav_header.magic_riff[1] != 'I' || wav_header.magic_riff[2] != 'F' || wav_header.magic_riff[3] != 'F'
//     || wav_header.magic_wavefmt_[0] != 'W' || wav_header.magic_wavefmt_[1] != 'A' || wav_header.magic_wavefmt_[2] != 'V' || wav_header.magic_wavefmt_[3] != 'E'
//     || wav_header.magic_wavefmt_[4] != 'f' || wav_header.magic_wavefmt_[5] != 'm' || wav_header.magic_wavefmt_[6] != 't' || wav_header.magic_wavefmt_[7] != ' '
//     || wav_header.format_type != 1 || wav_header.num_channels <= 0 /*|| wav_header.sample_rate != 44100*/ || wav_header.bits_per_sample != 16
//     || wav_header.magic_data[0] != 'd' || wav_header.magic_data[1] != 'a' || wav_header.magic_data[2] != 't' || wav_header.magic_data[3] != 'a'));

//     int16_t *samples = malloc (wav_header.data_size);
// 	assert (samples);

// 	assert (fread(samples, 1, wav_header.data_size, file) == wav_header.data_size);

//     if (wav_header.num_channels != 1) {
//         printf ("File: %s has more than one channel (%d). Mixing all channels into one\n", filename, wav_header.num_channels);
//         int32_t mix = 0;
//         for (int i = 0; i < wav_header.data_size / 2 / wav_header.num_channels; ++i) {
//             mix = 0;
//             for (int j = 0; j < wav_header.num_channels; ++j) {
//                 mix += (uint32_t)samples[i * wav_header.num_channels + j] + INT16_MIN;
//             }
//             mix /= wav_header.num_channels;
//             mix -= INT16_MIN;
//             samples[i] = (int16_t)mix;
//         } 
//     }

// 	int sample_count = wav_header.data_size / (wav_header.bits_per_sample / 8) / wav_header.num_channels;
//     sound_t *sound;

//     printf ("%s sample rate: %d\n", filename, wav_header.sample_rate);
//     if (wav_header.sample_rate != 22050) {
//         printf ("Resampling!\n");
//         float ratio = 22050.f / wav_header.sample_rate;
//         uint32_t resample_count = sample_count * ratio + .5f;
//         sound = malloc (sizeof(sound_t) + resample_count);
//         assert (sound);
//         sound->count = resample_count;
//         for (int i = 0; i < resample_count; ++i) {
//             float index = i * (1 / ratio);
//             float frac = index - (int)index;
//             float sample = samples[(int)index] * (1 - frac) + samples[(int)index + 1] * frac;
//             sound->samples[i] = (uint8_t)((sample + INT16_MAX) / UINT16_MAX * UINT8_MAX);
//         }
//     }
//     else {
//         sound = malloc (sizeof (sound_t) +  sample_count);
//         assert (sound);
//         sound->count = sample_count;

//         for (int s = 0; s < sample_count; ++s) {
//             int32_t sam;
//             sam = samples[s];
//             sam += INT16_MIN;
//             sam /= UINT16_MAX / UINT8_MAX;
//             assert (sam < 256);
//             sound->samples[s] = sam;
//         }
//     }

// 	free (samples);

// 	return sound;
// }

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