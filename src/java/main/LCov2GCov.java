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

public class LCov2GCov
{
    //Transform lcov report (by llvm-cov export -format=lcov) to gcov report (by gcov -i)
    public static void main(String[] args) {
	File lcov_f = new File(args[0]);
	System.out.println(getGCovString(lcov_f));
    }

    public static String getGCovString(File lcov_f) {
	List<String> lcov_flines = null;
	try { lcov_flines = FileUtils.readLines(lcov_f, (String) null); }
	catch (Throwable t) { System.err.println(t); t.printStackTrace(); }
	if (lcov_flines == null) { return null; }

	StringBuilder rslt_sb = new StringBuilder();
	List<String> fn_lines = new ArrayList<String>();
	int fnda_index = 0;
	for (String lcov_fline : lcov_flines) {
	    if (lcov_fline.startsWith("SF:")) {
		rslt_sb.append("file:" + lcov_fline.substring("SF:".length()));
	    }

	    else if (lcov_fline.startsWith("FN:")) {
		fn_lines.add(lcov_fline);
	    }

	    else if (lcov_fline.startsWith("FNDA:")) {
		String[] fn_elems = fn_lines.get(fnda_index).substring("FN:".length()).split(",");
		String[] fnda_elems = lcov_fline.substring("FNDA:".length()).split(",");
		if (!fn_elems[1].equals(fnda_elems[1])) {
		    System.err.println("Inconsistent function names: " + fn_elems[1] +
				       " and " + fnda_elems[1]);
		    System.err.println("Failed generating gcov string.");
		    return null;
		}

		rslt_sb.append("\nfunction:" + fn_elems[0]);
		rslt_sb.append("," + fnda_elems[0]);
		rslt_sb.append("," + fn_elems[1]);
		fnda_index += 1;
	    }

	    else if (lcov_fline.startsWith("DA:")) {
		String[] da_elems = lcov_fline.substring("DA:".length()).split(",");
		rslt_sb.append("\nlcount:");
		rslt_sb.append(da_elems[0] + "," + da_elems[1]);
	    }
	}

	return rslt_sb.toString();
    }
}
