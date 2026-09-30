# Dictionaries of the Tifa application

Everything in this folder is deployed next to the application (`bin/dict` on
Windows and Linux, `Tifa.app/Contents/Resources/dict` on macOS) and is read at
startup. The folder can be overridden in the main window; an override has to
contain the same layout.

## Layout

```
dict/
├── ds-zh-pinyin-lite.txt      # Mandarin: pinyin syllable -> phonemes
├── jyutping_dict.txt          # Cantonese: jyutping syllable -> phonemes
├── japanese_dict_full.txt     # Japanese: romaji -> phonemes
├── ds_cmudict-07b.txt         # English: word -> phonemes
├── LstmG2p-Eng/               # optional English out-of-vocabulary model
│   ├── encoder.onnx
│   ├── decoder.onnx
│   ├── char.json
│   └── phonemes.json
├── mandarin/                  # cpp-pinyin, deployed from the vcpkg share dir
│   ├── word.txt
│   └── trans_word.txt
└── cantonese/                 # cpp-pinyin, deployed from the vcpkg share dir
    ├── word.txt
    └── trans_word.txt
```

The four text dictionaries are shipped with the repository; they are the files
of the same name from the `dictionaries` folder of the
[TIFA](https://github.com/openvpi/TIFA) repository. `ds_cmudict-07b.txt` is
derived from [CMUdict](https://github.com/cmusphinx/cmudict) (BSD-2-Clause);
check the upstream licenses before redistributing this folder.

## Mandarin and Cantonese

The hanzi to pinyin and hanzi to jyutping conversion uses
[cpp-pinyin](https://github.com/pypinyin/cpp-pinyin) as the engine, then maps
the romanisation to phonemes with `ds-zh-pinyin-lite.txt` and
`jyutping_dict.txt`. The engine reads `mandarin/` and `cantonese/` itself;
`CMakeLists.txt` copies them from
`${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict` on
Windows. On other platforms copy them from your cpp-pinyin installation.

## Japanese

Only kana is converted (`japanese_dict_full.txt` maps romaji to phonemes).
Kanji needs a morphological analyser (MeCab with UniDic in the Python
pipeline) which is not available here, so kanji is reported as unsupported
instead of being aligned with wrong phonemes. Write the kana reading, or add
the word to the text as kana, before aligning.

## English

`ds_cmudict-07b.txt` covers the common words. Words missing from it are
converted by the LSTM model in `LstmG2p-Eng` when it is present. Download the
model package from
[LstmG2p v1.0.0](https://github.com/wolfgitpr/LstmG2p/releases/tag/v1.0.0)
and extract `encoder.onnx`, `decoder.onnx`, `char.json` and `phonemes.json`
into `dict/LstmG2p-Eng`. These files are not part of the repository; without
them an unknown English word is reported as out of vocabulary and the file is
failed or the pronunciation is dropped, depending on the *Unknown phonemes*
option of the main window.
