# Copilot Instructions

## Project Guidelines
- For this plugin, prefer auto-typed sample-path locals without Hungarian-style d/f prefixes when adapting audio sample precision.
- Prefer float for internal DSP state but avoid bottlenecking host input/output precision; keep the direct audio I/O path adaptive to the host buffer type.
- Prefer the member name `fSampleRate` rather than `sampleRate` for the processor sample-rate field.

## Language Preferences
- Use UK English wording throughout the code and comments.