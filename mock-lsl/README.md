# Creating an Random Data LSL Stream for Testing
## Setting Up
From this directory, run the following commands in a terminal to set up the environment

### Linux
- `python3 -m venv .venv`
- `. .venv/bin/activate`
- `python -m pip install pylsl`

## Running the stream
Run the following:
- `python -m pylsl.examples.SendData -s 1000 -n lsl_property_value -t lsl_property`

Once this is running, the c++ tests can be run successfully. The tests should also be run while there is no active LSL stream.

## More Info
See https://github.com/labstreaminglayer/pylsl/tree/main/src/pylsl/examples