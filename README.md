# example arduino <--> python serial clock

Download these files (Green 'Code' button, then download .zip), and save them somewhere seperate to your Clock design sketch that you've been working on (don't overwrite it or you will lose your UI design).

## Steps

1. You should already have Python installed. You can check by opening up a terminal and running `python -v` or `python --version`. If this doesn't work, try running `python3 --version`. The installed version should then be printed on the command line.
2. Now you need to check that you can run `pip` from the command line. `pip` is Python's package manager. Follow these steps to [ensure you can run pip from the command line](https://packaging.python.org/en/latest/tutorials/installing-packages/#ensure-you-can-run-pip-from-the-command-line).
3. Install the `pyserial` library. Run `pip install pyserial` or `python3 -m pip install pyserial`
4. You should now be able to run your Python script! `cd` to your working directory (or go to Finder/File Explorer and click 'Open in Terminal' or 'New terminal at folder') and then run `python send_days.py` or `python3 send_days.py`
