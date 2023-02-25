# Game Test Automation

In this project, a framework based on computer vision methods is developed that can be used to automate the process of game testing.
The goal is to replace the parts of the testing process that require human users (testers) with machines as much as possible in order to reduce costs and perform more tests in less time by scaling hardware resources.

It is a reusable prototype framework architecture and implementation that is independent of the engine and deployment platform (i.e., it works on different devices such as PCs, game consoles, or different operating systems) and allows users to directly reuse existing open source computer vision library code.

The documentation can be found in the Doc directory, and the tool can be found in the Code directory.

A demo video of our tool can be viewed online here:
https://youtu.be/qFfWvaLtOU0.


#### How to start testing with our toolset for your game ?

Step 1: Check the BDD examples implemented in Unity and Python. in folder Code/bdd.
Note: while implemented in Unity as a proof of concept, our set of tools is independent of any engine implementation and can be used for Unreal, or any other custom engine.

Step 2: For sound testing tools check: Code/SoundTestingSupport

Step 3: For animation testing tools check: Code/AnimationtestingSupport

Step 4: For computer vision models and tests check: Code/TestApp and TestsPriorityApp folders.
