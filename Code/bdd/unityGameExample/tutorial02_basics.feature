Feature: Input testing

@Controls
Scenario: Check top camera input
    Given we start a game instance
        And we loaded sub-level CaptureCoinsGood
        And we pressed TopCameraKey
    When we pressed TopCameraKey
    Then we see 4 coins




