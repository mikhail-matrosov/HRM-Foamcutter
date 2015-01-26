import cv2
import numpy as np
from random import random

def rndCol():
    return (int(255*random()), int(255*random()), int(255*random()))

def drawPath(img, paths, thickness, drawJumps=1):
    
    img = cv2.cvtColor(img, cv2.COLOR_GRAY2RGB) 
    
    for path in paths:
        ps = map(tuple, path)
        for i in range(0,path.shape[0]-1):
            cv2.line(img, ps[i], ps[i+1], (0,)*3, thickness, cv2.CV_AA)
        #cv2.circle(img, ps[0], 3, (255,0,0))
        #cv2.circle(img, ps[-1], 2, (0,0,255))

    # jumps
    if drawJumps:
        for i in range(len(paths)-1):
            p1 = tuple(paths[i][-1])
            p2 = tuple(paths[i+1][0])
            cv2.line(img, p1, p2, rndCol(), thickness)
    
    return img


def plot_from_file(INPUT_FILE, drawJumps=1):
    data = np.load(INPUT_FILE)
    
    img = (np.ones(data[0])*255).astype(np.uint8)
    img = drawPath(img, data[1], 1, drawJumps)
    
    cv2.imwrite(INPUT_FILE+'_p.png', img)
    cv2.imshow('result', img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()