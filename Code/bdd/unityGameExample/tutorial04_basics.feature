Feature: Sound testing

@Sound
Scenario: Check dialogs
    Given we start a game instance
        And we loaded sub-level {MissionId}
    When {EntityId} started {BehaviorId}
    Then we should hear {SoundId} with similarity above {SimT}

    Examples:
        | MissionId | EntityId | BehaviorId    | SoundId            |SimT  |
        | mission1 | aircraftW | startEngine   | snd_startAirEngine | 0.75 |
        | mission2 | Wedge     | startComment1 | comment_1          | 0.8  |
        | mission2 | gameMusic | missionStart  | snd_backgroundMiss2| 0.75 |
        | mission1 | aicraftW  | hitByEnemy    | comment_2          | 0.8  |
        | mission2 | aicraftW  | crashed       | snd_crashAirplane  | 0.75 |

