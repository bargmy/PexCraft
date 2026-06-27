Text input patch notes
======================

- SDL text input is no longer started at app launch, so Android should not open the soft keyboard immediately.
- Text input starts only when chat opens or when the user selects/clicks an editable field.
- Text input fields now request an SDL text-input rectangle so mobile/TV IME placement has a target area.
- Android TV and LG webOS remote OK can trigger the keyboard on text-entry screens instead of immediately submitting.
- Text input is stopped when leaving the current screen.
