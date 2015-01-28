"""
@Authors: mikhail-matrosov and Alexey Boyko
"""
from main import *
from animate import *
import Tkinter, tkFileDialog

print 'Select input image'
root = Tkinter.Tk()
root.withdraw()
inputImagePath = tkFileDialog.askopenfilename()

# layers=1 : trace several layers (MM style)
# layers=0 : trace single lines (A.I.BO style)
fname = main(inputImagePath)

print 'Now printing.'
animate(fname, comPort='COM19')