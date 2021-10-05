from behave import given, when, then
from gameModel import GameModel
from hamcrest import assert_that, equal_to
import asyncio


@given("we start a game instance")
def step_impl(context):
    pass


@given("we loaded sub-level Mission2")
def step_impl(context):
    pass

@when('Wedge started commentId12_voice')
def step_impl(context):
    pass

@then("we should hear text commentId12_sound with similarity above 0.8")
def step_impl(context):
    pass

@then("we should hear {SoundId} with similarity above {SimT}")
def step_impl(context):
    def checkSoundSimilarity(context, SoundId, SimT):
        assert context.sndCheck.CheckSim(SoundId, context.recordedSound) >= SimT

