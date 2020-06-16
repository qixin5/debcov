package edu.gatech.cc.debcov;

import java.util.List;
import java.util.ArrayList;
import java.util.Set;
import java.util.HashSet;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.io.File;
import org.apache.commons.io.FileUtils;


public class PathCoverage
{
    public String fname; //file name
    public Map<Integer, Integer> fcmap; //function-count map
    public Map<Integer, Integer> lcmap; //line-count map

    public PathCoverage(String _fname, Map<Integer, Integer> _fcmap, Map<Integer, Integer> _lcmap) {
	fname = _fname;
	fcmap = _fcmap;
	lcmap = _lcmap;
    }

    public String getFileName() { return fname; }
    
    public Map<Integer, Integer> getFunctionCountMap() { return fcmap; }

    public Map<Integer, Integer> getLineCountMap() { return lcmap; }

    //Check if every line from pc is covered by this coverage
    public boolean coversByLines(PathCoverage pc) {
	Map<Integer, Integer> pc_lcmap = pc.getLineCountMap();
	Iterator<Map.Entry<Integer, Integer>> pc_lc_itr = pc_lcmap.entrySet().iterator();
	while (pc_lc_itr.hasNext()) {
	    Map.Entry<Integer, Integer> pc_lc_entry = pc_lc_itr.next();
	    if (pc_lc_entry.getValue().intValue() > 0) { //If the line is covered
		if (lcmap.get(pc_lc_entry.getKey().intValue()) == null ||
		    lcmap.get(pc_lc_entry.getKey().intValue()).intValue() <= 0) {
		    return false;
		}
	    }
	}
	return true;
    }

    //Check if every function from pc is covered by this coverage
    public boolean coversByFunctions(PathCoverage pc) {
	Map<Integer, Integer> pc_fcmap = pc.getLineCountMap();
	Iterator<Map.Entry<Integer, Integer>> pc_fc_itr = pc_fcmap.entrySet().iterator();
	while (pc_fc_itr.hasNext()) {
	    Map.Entry<Integer, Integer> pc_fc_entry = pc_fc_itr.next();
	    if (pc_fc_entry.getValue().intValue() > 0) { //If the line is covered
		if (fcmap.get(pc_fc_entry.getKey().intValue()) == null ||
		    fcmap.get(pc_fc_entry.getKey().intValue()).intValue() <= 0) {
		    return false;
		}
	    }
	}
	return true;
    }

    public String toString() {
	StringBuilder sb = new StringBuilder();
	sb.append("file:" + fname);
	for (Integer l : fcmap.keySet()) {
	    sb.append("\nfunction:" + l.intValue() + "," + fcmap.get(l).intValue());
	}
	for (Integer l : lcmap.keySet()) {
	    sb.append("\nlcount:" + l.intValue() + "," + lcmap.get(l).intValue());
	}
	return sb.toString();
    }
}
