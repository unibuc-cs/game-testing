from behave import given, when, then
from gameModel import GameModel
from hamcrest import assert_that, equal_to
import asyncio


@given("we start a game instance")
def step_impl(context):
    context.gameModel = GameModel()
    asyncio.run(context.gameModel.connectToGame())
    #print("connected ")


@given("we loaded sub-level {SubLevelName}")
def step_impl(context, SubLevelName):
    asyncio.run(context.gameModel.load_subLevel(SubLevelName))

@when('we want to collect all items in less than {timeLimit} seconds')
def step_impl(context, timeLimit):
    context.timeLimit = int(timeLimit)
    asyncio.run(context.gameModel.testPickup(context))
    assert True is not False

@then("remaining number of coins is {numItems}")
def step_impl(context, numItems):
    assert_that(context.gameModel.getNumPickupsRemaining(), equal_to(int(numItems)))

