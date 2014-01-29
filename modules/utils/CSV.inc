<?php
/**
 * CSV export
 */

class CSV{
	
	/**
	 * @param $file: file name without path , ex: report.csv
	 * @param $data: data list [{},{}...]
	 * @param $titles: {colName1:label1, colName2:label2...}
	 */
	public static function export($file,$data,$titles=null){
		header('Content-Encoding: UTF-8');
		header('Content-type: text/csv; charset=UTF-8');
		if(!str_has($file, ".csv"))
			$file .= ".csv";
        header( 'Content-Disposition: attachment;filename='.$file);
        $fp = fopen('php://output', 'w');
        
        if($titles == null){
        	$cols = array();
        	foreach ($data as $d){
        		$cols = array_unique(array_merge($cols, array_keys($d)));
        	}
        	$titles=array();
        	foreach ($cols as $col)
        		$titles[$col] = $col;
        }
        //print titles
        fputcsv($fp, array_values($titles));
		foreach ($data as &$d){
			$row = array();
			foreach ($titles as $col=>$label){
				$v = hash_get($d, $col);
				$row[] = $v;
			}
			fputcsv($fp, $row);
		}
        fclose($fp);
	}

}