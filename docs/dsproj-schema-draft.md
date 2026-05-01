# .dsproj Data Model Draft

**Task**: TASK-2.5  
**Status**: Draft  
**References**: (original planning documents, no longer available)

---

## 1. `defaults.models` JSON Schema

The `defaults.models` object lives inside the top-level `defaults` field of a `.dsproj` file. Each key corresponds to a pipeline step that requires model configuration.

### 1.1 Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "defaults.models",
  "description": "Default model configuration for each pipeline step in a .dsproj file.",
  "type": "object",
  "properties": {
    "midi": {
      "description": "GAME audio-to-MIDI transcription model (Step 7).",
      "type": "object",
      "properties": {
        "path":     { "type": "string", "description": "Path to model directory (relative to app dir or absolute)." },
        "provider": { "type": "string", "enum": ["cpu", "dml", "cuda"], "default": "cpu" },
        "deviceId": { "type": "integer", "minimum": 0, "default": 0 }
      },
      "required": ["path"]
    },
    "build_ds": {
      "description": "RMVPE pitch extraction model (Step 8).",
      "type": "object",
      "properties": {
        "path":     { "type": "string", "description": "Path to RMVPE ONNX file." },
        "provider": { "type": "string", "enum": ["cpu", "dml", "cuda"], "default": "cpu" },
        "deviceId": { "type": "integer", "minimum": 0, "default": 0 }
      },
      "required": ["path"]
    },
    "alignment": {
      "description": "HuBERT forced alignment model (Step 4).",
      "type": "object",
      "properties": {
        "path":     { "type": "string", "description": "Path to HuBERT FA ONNX file." },
        "provider": { "type": "string", "enum": ["cpu", "dml", "cuda"], "default": "cpu" },
        "deviceId": { "type": "integer", "minimum": 0, "default": 0 }
      },
      "required": ["path"]
    },
    "asr": {
      "description": "FunASR lyric recognition model (Step 2).",
      "type": "object",
      "properties": {
        "path":     { "type": "string", "description": "Path to FunASR ONNX file." },
        "provider": { "type": "string", "enum": ["cpu", "dml", "cuda"], "default": "cpu" },
        "deviceId": { "type": "integer", "minimum": 0, "default": 0 }
      },
      "required": ["path"]
    }
  },
  "additionalProperties": false
}
```

### 1.2 Example

```json
{
  "defaults": {
    "models": {
      "asr": {
        "path": "models/asr/funasr_cn.onnx",
        "provider": "dml",
        "deviceId": 0
      },
      "alignment": {
        "path": "models/alignment/hubert_fa.onnx",
        "provider": "dml",
        "deviceId": 0
      },
      "midi": {
        "path": "models/game/",
        "provider": "dml",
        "deviceId": 0
      },
      "build_ds": {
        "path": "models/rmvpe/rmvpe.onnx",
        "provider": "cpu",
        "deviceId": 0
      }
    }
  }
}
```

### 1.3 Notes

- Keys match the `.dsitem` `step` field values from `08-project-format.md`.
- `provider` and `deviceId` are optional. When omitted, the runtime falls back to `"cpu"` / `0`.
- Steps that don't use ML models (`slicer`, `build_csv`) and manual editing steps (`asr_review`, `alignment_review`, `pitch_review`) have no entry here. Slicer parameters live separately under `defaults.slicer` (see `08-project-format.md` §4.1).
- The `name` and `version` fields from the existing `.dsitem` model object are intentionally omitted from `defaults.models`. Those are runtime metadata written into `.dsitem` files when a step executes, not user configuration.

---

## 2. `dstemp/` Directory Mapping

This table maps Phase 2 modules to their output locations within the `dstemp/` directory tree and the corresponding `.dsitem` step identifiers.

| Module | Output | `dstemp/` subdir | `.dsitem` step |
|--------|--------|-------------------|----------------|
| TextGridToCsv | `transcriptions.csv` | `csv/` | `build_csv` |
| PhNumCalculator | `transcriptions.csv` (ph_num column added) | `csv/` | `build_csv` |
| game-infer align | `transcriptions.csv` (MIDI columns appended: note_seq, note_dur, note_slur, note_glide) | `midi/` | `midi` |
| CsvToDsConverter | `*.ds` | `ds/` | `build_ds` |
| CsvToDsConverter (RMVPE callback) | `*.f0.npy` | `f0/` | `build_ds` |

### How the steps chain together

```
alignment/*.TextGrid
        │
        ▼
  TextGridToCsv ──► csv/transcriptions.csv   (ph_seq, ph_dur)
        │
        ▼
  PhNumCalculator ─► csv/transcriptions.csv   (+ ph_num)
        │                                       ▲
        ▼                                       │
  game-infer align ► csv/transcriptions.csv   (+ note_seq, note_dur, note_slur, note_glide)
        │               written back to midi/ dsitem
        ▼
  CsvToDsConverter ──► ds/*.ds + f0/*.f0.npy
```

TextGridToCsv and PhNumCalculator both write to `csv/` and share a single `transcriptions.dsitem` with step `build_csv`. PhNumCalculator runs as a sub-step of Build CSV, so they share the same `.dsitem` record.

The `f0/` directory holds cached F0 curves extracted by RMVPE during Build DS. These files have no dedicated `.dsitem`; the `ds/` directory's `.dsitem` (step `build_ds`) records the RMVPE model used.

---

## 3. DsDocument JSON / TranscriptionRow Field Mapping

### 3.1 Field-by-field correspondence

| TranscriptionRow field | DS JSON sentence field | Type | Intermediate only? | Notes |
|------------------------|----------------------|------|:-------------------:|-------|
| `name` | (not stored in sentence) | `QString` | Yes | Maps to `.dsitem` sourceFile via itemId. DS JSON uses `offset` to locate the sentence within the audio file. |
| `phSeq` | `ph_seq` | space-separated `QString` | No | Phoneme sequence including SP/AP markers. |
| `phDur` | `ph_dur` | space-separated `QString` (floats) | No | Duration of each phoneme in seconds. |
| `phNum` | `ph_num` | space-separated `QString` (ints) | No | Number of phonemes per word. Used by DiffSinger for word boundary inference. |
| `noteSeq` | `note_seq` | space-separated `QString` | No | Note names (e.g. `C4`, `rest`). |
| `noteDur` | `note_dur` | space-separated `QString` (floats) | No | Duration of each note in seconds. |
| `noteSlur` | `note_slur` | space-separated `QString` (0/1) | No | Slur flag per note. |
| `noteGlide` | `note_glide` | space-separated `QString` | No | Glide type per note. Optional in both formats. |
| (none) | `offset` | `double` | — | DS JSON only. Start time of the sentence within the audio file, in seconds. Computed by CsvToDsConverter from slice timing. |
| (none) | `f0_seq` | space-separated `string` | — | DS JSON only. F0 curve sampled at `f0_timestep` intervals. Written by CsvToDsConverter using RMVPE output. |
| (none) | `f0_timestep` | `double` | — | DS JSON only. Time step between F0 samples (typically `hopSize / sampleRate`). |

### 3.2 Lifecycle

TranscriptionRow is the intermediate transfer format between Steps 6, 7, and 8. Fields accumulate as data flows through the pipeline:

1. **After Step 6 (Build CSV)**: `name`, `phSeq`, `phDur`, `phNum` are populated.
2. **After Step 7 (GAME align)**: `noteSeq`, `noteDur`, `noteSlur`, `noteGlide` are appended.
3. **Step 8 (Build DS)**: CsvToDsConverter reads all TranscriptionRow fields, adds `offset`, `f0_seq`, `f0_timestep`, and writes the complete DS JSON sentence.

### 3.3 Round-trip safety

DsDocument stores raw `nlohmann::json` objects. Any fields beyond those listed above are preserved on save without modification. CsvToDsConverter constructs JSON objects directly and pushes them into `DsDocument::sentences()`, so custom or future fields added by downstream tools (e.g. PitchLabeler's `note_glide`, `f0_seq` edits) survive round-trips through DsDocument.

TranscriptionRow, by contrast, has a fixed set of fields. Unknown CSV columns are silently dropped on read. This is acceptable because the CSV is a transient intermediate file, not a long-term storage format.
