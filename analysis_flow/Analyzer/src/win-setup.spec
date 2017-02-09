# -*- mode: python -*-

python_path_add2 = "src\windows"
python_path_add3 = "src\windowhandlers"
python_path_add4 = "src\objects"
python_path_add5 = "src\images"

a = Analysis(["analyzer.py"],
             pathex=[python_path_add2, python_path_add3, python_path_add4, python_path_add5],
             hiddenimports=[],
             hookspath=None,
             runtime_hooks=None)

pyz = PYZ(a.pure)

exe = EXE(pyz,
          a.scripts,        
          exclude_binaries=True,
          name="analyzer.exe",
          debug=True,
          strip=None,
          upx=True,
          console=True)

coll = COLLECT(exe,					
			a.binaries,
			a.zipfiles,
			a.datas + '',
			strip=None,
			upx=True,
			name="Analyzer")
             
