// Copyright [2025] [Nicholas Walton]
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "midi.h"

uint8_t MidiKeyToNote (midi_key_t key) {
    assert ((key.key >= 'a' && key.key <= 'g') || (key.key >= 'A' && key.key <= 'G'));
    assert (key.octave >= 0);
    assert (key.octave <= 9);
    constexpr uint8_t k[] = {['c']=0,['C']=0,['d']=2,['D']=2,['e']=4,['E']=4,['f']=5,['F']=5,['g']=7,['G']=7,['a']=9,['A']=9,['b']=11,['B']=11};
    return key.octave * MIDI_KEY_MIN + MIDI_KEYS_PER_OCTAVE + k[(uint8_t)key.key] + key.sharp;
}

midi_key_t MidiNoteToKey (uint8_t note) {
    assert (note >= MIDI_KEY_MIN);
    assert (note <= MIDI_KEY_MAX);
    constexpr char k[MIDI_KEYS_PER_OCTAVE] = {'C','C','D','D','E','F','F','G','G','A','A','B'};
    constexpr bool sharp[MIDI_KEYS_PER_OCTAVE] = {0,1,0,1,0,0,1,0,1,0,1,0};
    const uint8_t index = (note-MIDI_KEY_MIN)%MIDI_KEYS_PER_OCTAVE;
    return (midi_key_t) {
        .key = k[index],
        .octave = note / MIDI_KEYS_PER_OCTAVE - 1,
        .sharp = sharp[index],
    };
}

uint8_t MidiFrequencyToNearestNote (uint16_t frequency) {
    int i = 0;
    while (i < MIDI_NOTE_COUNT && midi_note_frequency[i] < frequency) ++i;
    if (i == MIDI_NOTE_COUNT) return MIDI_NOTE_COUNT-1;
    float dista = frequency - midi_note_frequency[i];
    float distb = midi_note_frequency[i+1] - frequency;
    if (dista < distb) return i;
    return i+1;
}