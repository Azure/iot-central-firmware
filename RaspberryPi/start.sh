# check to see if libboost-python is installed
OUT=`dpkg -s libboost-python1.62.0`
if [ $? != 0 ]; then
   sudo apt-get install libboost-python1.62.0
fi

cd src
python main.py
