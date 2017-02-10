
all: 
	echo "Usage is: make windows OR make osx"

osx:	
	pyuic4 src/windows/AddAction.ui > src/windows/wAddAction.py
	pyuic4 src/windows/AddVariable.ui > src/windows/wAddVariable.py
	pyuic4 src/windows/Main.ui > src/windows/wMain.py	
	pyrcc4 src/images/images.qrc > src/images/images_rc.py
	rm -f src/images/*.pyc	
	rm -f src/windowhandlers/*.pyc
	rm -f src/windows/*.pyc
	rm -f src/objects/*.pyc
	
osx-dist: osx		
	pyinstaller --clean --windowed src/osx-setup.spec	
	
win:
	pyuic4 src\windows\AddAction.ui > src\windows\wAddAction.py
	pyuic4 src\windows\AddVariable.ui > src\windows\wAddVariable.py
	pyuic4 src\windows\Main.ui > src\windows\wMain.py
	pyrcc4 src\images\images.qrc > src\images\images_rc.py
	
	del src\windows\*.pyc
	del src\windowhandlers\*.pyc
	del src\objects\*.pyc
	del src\images\*.pyc
	
win-dist: win	
	pyinstaller --clean --windowed --onedir --icon="src\images\logo2.ico" src\win-setup.spec	