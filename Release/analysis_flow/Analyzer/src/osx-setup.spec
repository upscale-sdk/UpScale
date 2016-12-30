# -*- mode: python -*-

python_path_add0 = './analyzer/'
python_path_add1 = './analyzer/src'
python_path_add2 = './analyzer/src/windows'
python_path_add3 = './analyzer/src/windowhandlers'
python_path_add4 = './analyzer/trunk/analyzer/src/objects'
python_path_add5 = './analyzer/trunk/analyzer/src/images'

a = Analysis(['src/analyzer.py'],
             pathex=[python_path_add0, python_path_add1, python_path_add2, python_path_add3, python_path_add4, python_path_add5],
             hiddenimports=[],
             hookspath=None,
             runtime_hooks=None)

pyz = PYZ(a.pure)

exe = EXE(pyz,
          a.scripts,        
          exclude_binaries=True,
          name='analyzer',
          debug=True,
          strip=None,
          upx=True,
          console=True,
          icon='src/images/app_icon.icns')

coll = COLLECT(exe,					
			a.binaries,
			a.zipfiles,
			a.datas + '',
			strip=None,
			upx=True,
			name='Analyzer')
             
#app = BUNDLE(coll,
#        	name='Analyzer.app',
#        	icon='src/images/app_icon.icns')
        	
