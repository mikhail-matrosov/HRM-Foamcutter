"""
@Authors: mikhail-matrosov and Alexey Boyko
@Created August, 20

This script takes image and converts it into engraving, a path that also can 
be drawn with a pen or a marker.
"""

import cv2
import numpy as np
from findPaths import *
from plotter import plot_from_file

def main(inputImagePath):
    print "main"
    
    # open
    img = cv2.imread(inputImagePath)
    if len(img.shape)>2:
        img = cv2.cvtColor(img, cv2.COLOR_RGB2GRAY)
    
    w, h = img.shape

    paths = sortPaths(findPaths(img))
    
    print 'Done.'
    printPathStatistics(paths, img.shape)
    
    parts = inputImagePath.split('/')
    parts[-1] = 'r_'+parts[-1]
    fname = '/'.join(parts)+'.npy'
    np.save(fname, ((w,h), np.array(paths)))
    plot_from_file(fname, 0)
    return fname
    
def showpic(image):
    cv2.imshow('',image)
    
    k = cv2.waitKey(0)
    if k == ord('s'): # wait for 's' key to save and exit
        cv2.imwrite('ololo.png',image)
    cv2.destroyAllWindows()
#main('aysilu.JPG')