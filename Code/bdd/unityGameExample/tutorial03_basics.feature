Feature: Sound testing

@Sound
Scenario: Check dialogs
    Given we start a game instance
        And we loaded sub-level Mission2
        And we pressed TopCameraKey
    When Wedge started commentId12_voice
    Then we should hear text commentId12_sound with similarity above 0.8
