Feature: Capturing coins

@Physics
Scenario: Capture all coins
    Given we start a game instance
        And we loaded sub-level CaptureCoinsGood
    When we want to collect all items in less than 20 seconds 
    Then remaining number of coins is 0




