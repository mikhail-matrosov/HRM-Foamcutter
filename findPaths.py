import numpy as np
import cv2

def unfoldPath(path):
    p = path[:,0]*10000 + path[:,1]
    ixs = np.array(np.where(p[:-2]==p[2:])).flatten()+1
    if len(ixs) == 1:
        ix = ixs[0]
        if ix*2>len(p):
            return path[ix:, :]
        else:
            return path[:ix+1, :]
    elif len(ixs) == 2:
        return path[ixs[0]:ixs[1]+1, :]
    return path

def removeDoubles(img, paths):
    z = np.zeros((img.shape[1], img.shape[0]))
    cleans = []
    
    for path in paths:
        t = map(tuple, path)
        
        #I = np.array([1]+map(lambda p: z[p], t)+[1])
        I = np.ones(2+len(t))
        for i in range(len(t)):
            I[i+1] = z[t[i]]
            z[t[i]] = 1
        
        starts = I[:-1] * (1-I[1:]) # starts and ends of chunks
        ends = (1-I[:-1]) * I[1:]
        
        # find its indexes
        ss = np.where(starts)[0]
        es = np.where(ends)[0]
        
        # add each individual chunk
        for s,e in zip(ss,es):
            cleans += [path[s:e]]
        
        # mark visited points
        for p in t:
            z[p] = 1
        
    return cleans

def sortPaths(paths):
    if len(paths)<2:
        return paths
    
    p = paths.pop()
    firsts = map(lambda p: p[0], paths)
    lasts = map(lambda p: p[-1], paths)
    
    pathsSorted = []
    while len(paths)>0:
        # find the closest another end
        df = np.linalg.norm(firsts-p[-1],2,1)
        dl = np.linalg.norm(lasts-p[-1],2,1)
        
        mf = df.argmin()
        ml = dl.argmin()
        
        if df[mf] < dl[ml]:
            p = paths.pop(mf)
            pathsSorted += [p]
            firsts.pop(mf)
            lasts.pop(mf)
        else:
            p = paths.pop(ml)[::-1]
            pathsSorted += [p]
            firsts.pop(ml)
            lasts.pop(ml)
            
    return pathsSorted

def findPaths(img):
    (q,h)=cv2.findContours((img>127).astype(np.uint8), mode=cv2.RETR_LIST, method=cv2.CHAIN_APPROX_NONE)
    
    # there's also workaround the fact, that path is traced in both directions
    paths = [p.squeeze() for p in q if p.shape[0]>4]
    paths = removeDoubles(img, paths)
    paths = sortPaths(paths)
    
    # connect broken paths
    ds = [np.linalg.norm(paths[i][-1]-paths[i+1][0]) for i in range(len(paths)-1)]
    ix = np.where(np.array(ds)<1.5)[0]
    
    for i in ix[::-1]:
        paths[i] = np.vstack((paths[i], paths.pop(i+1)))
    
    return paths

def printPathStatistics(paths, shape):
    # path length
    plen = sum(map(lambda p: np.linalg.norm(p[:-1]-p[1:], 2, 1).sum(), paths))
    # flight length
    flen = sum(np.linalg.norm(paths[i][-1]-paths[i+1][0]) for i in range(len(paths)-1))
    
    M = max(shape[0], shape[1])
    m = min(shape[0], shape[1])
    ar = 1.0*M/m
    
    if ar>1.414:
        scale = 0.297/M
    else:
        scale = 0.21/m
    
    print 'Path length is %d px (%.2f m)' % (np.round(plen), plen*scale)
    print 'Flight length is %d px (%.2f m)' % (np.round(flen), flen*scale)