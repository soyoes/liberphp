<?php

/**
 *	@file: Dates.inc	
 *	@author: Soyoes 2011/04/27
 *	@uses:
 *	@example: 
 *****************************************************************************/

class Dates{
	
	const DB_FORMAT = "Y-m-d H:i:s";
	const Ymd_FORMAT = "Y-m-d";
	static $weekDays = array("Sunday","Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday");
	
	
	//example Sep => 09, June => 06
	static function textual2NumericMonth ($mon){
		$months = array("Jan","Feb","Mar","Apr","May","Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec");
		$monthFull = array("January","February","March","April","May","June","July","August","September","October","November","December");
		$idx = array_search($mon, $months);
		if($idx===false){
			$idx = array_search($mon, $monthFull);
		}
		if($idx===false)
			return "00";
		$idx ++;
		return $idx < 10 ? "0".$idx : "".$idx;
	}
	
	/**
	 * get weekday specified by $wday's number, witch includes ms.
	 * @return ms
	 * @param long $time : ms
	 * @param int $wday : 0=Sunday, 6=Saturday
	 */
	static function weekDay($wday, $time=null, $sundayFirst=true){
		if(!isset($time)) $time = self::ms()/1000;
		if(!$sundayFirst){
			$e = array_shift(self::$weekDays);
			self::$weekDays = array_merge(self::$weekDays,array($e));
		}
		$prefix = $wday<date("w") ? "last":"this"; 
		return date(self::Ymd_FORMAT,strtotime($prefix." ".self::$weekDays[$wday], $time));
	}

	static function firstOfMonth($time=null) {
		return isset($time)?
			date(self::Ymd_FORMAT, strtotime(date('Y', $time).'-'.date('m', $time).'-01 00:00:00')):
			date(self::Ymd_FORMAT, strtotime(date('Y').'-'.date('m').'-01 00:00:00'));
	}
	
	
	static function lastOfMonth($time=null) {
		return isset($time)?
			date(self::Ymd_FORMAT, strtotime('-1 second',strtotime('+1 month',strtotime(date('Y', $time).'-'.date('m', $time).'-01 00:00:00')))):
			date(self::Ymd_FORMAT, strtotime('-1 second',strtotime('+1 month',strtotime(date('Y').'-'.date('m').'-01 00:00:00'))));
	}
	
	static function timezoneDate($date, $timezone, $targetZone=0){
		if($timezone==$targetZone)
			return $date;
		$diff = $targetZone-$timezone;
		$date = date(self::DB_FORMAT, strtotime("$diff hour", strtotime($date)));
		return $date;
	}

	static function comparator($sortKey, $sortOrder){
		$cfmt = '$av=$a["%s"];if(!isset($av))$av=0;$bv=$b["%s"];if(!isset($bv))$bv=0;if($av==$bv){return 0;} return(strtotime($av)>strtotime($bv))?-1*%d:1*%d;';
		$code = sprintf($cfmt, $sortKey, $sortKey, $sortOrder, $sortOrder,$sortOrder);
		$cmp = create_function('$a, $b', $code);
		return $cmp;
	}
	
	static function startEndDayOfMonthlyCalendar($time, $format="time"){
		$_1st = Dates::firstOfMonth($time);
		$_31th = Dates::lastOfMonth($time);
		
		$startDay = strtotime("last monday",strtotime($_1st));
		$endDay = strtotime("next sunday",strtotime($_31th));
		if($format=="time")
			return array($startDay,$endDay);
		else
			return array(date($format,$startDay),date($format,$endDay));
	}
	
	/**
	 * for available time matching
	 * $times = [{"startAt":$start1,"endAt":$end1...},{"startAt":$start2,"startAt":$end2...}...] //$start & $end are timestamp
	 * $valueField = assign value to each cell of matrix. and value = $times[N][$valueFields] 
	 * return 
	 * {
			"20120101":{
		  		"01": ["12","13"...],	// values of 2012-01-01:01~02 , value is assigned by $times[N][$valueFields];
		  		"08": ["12","13"...],	// values of 2012-01-01:08~09 , value is assigned by $times[N][$valueFields];
		  		"09": ["12","13"...],	// values of 2012-01-01:09~10 , value is assigned by $times[N][$valueFields];
		  		"10": ["12","13"...],	// values of 2012-01-01:10~11 , value is assigned by $times[N][$valueFields];
		  	},
		  	"20120104":{
						  	
		  	}
		   	...			
	   }
	 * 
	 * 
	 * 
	 * */
	static function matrix($times,$valueField){
		$matrix = array();
		//drop pin
		foreach ($times as $e){
			$start = (int) $e["startAt"];
			$start -= $start % 3600; 
			$end = (int) $e["endAt"];
			$end += ($end % 3600)>0? 3600 - ($end % 3600) : 0;
			
			while ($start < $end){
				$d = date("Ymd", $start);
				$h = date("H", $start);
				Arrays::createIfNull($matrix, $d, array());
				Arrays::createIfNull($matrix[$d], $h, array());
				$matrix[$d][$h] []= $e[$valueField];
				$start += 3600;
			}
			
		} 
		return $matrix;
	}
	
	
	/**
	 * tell whether a time span drops its shadow in the given matrix
	 * */
	static function match($times, $start, $end, $valueField, &$matrix=null){
		if(empty($times))
			return array();

		$start -= $start % 3600; 
		$end += ($end % 3600)>0? 3600 - ($end % 3600) : 0;
		
		$hits = array();
		
		while ($start < $end){
			$d = date("Ymd", $start);
			$h = date("H", $start);
			if(isset($matrix[$d][$h])){
				$hits = array_merge($hits,$matrix[$d][$h]); 
			}
			$start += 3600;
		}
		$hits = array_unique($hits);
		return $hits;
		
	}
	
	//start&end is time
	static function daysBetween($start, $end){
		$t = $start;
		$days = array();
		while ($t < $end){
			$days[] = date('Y-m-d', $t);
			$t+=86400;
		}
		$endDay = date('Y-m-d', $end);
		if(!in_array($endDay, $days))
			$days[] = $endDay;
		return $days;
	}
	
	static function unix2utc($unix) {
		return (gmdate("Y-m-d", $unix) . "T" . gmdate("H:i:s", $unix) . "Z");
	}
	
	static function isSameDay($t1,$t2=null){
		if($t2 == null)
			$t2 = time();
		return date('d',$t1)==date('d',$t2);
	}
	
	static function getAge($time){
		if(str_has($time, "-"))
			$time = strtotime($time);
		$y = date('Y',$time);
        $age = (date("md", date("U", $time)) > date("md") ? ((date("Y")-$y)-1):(date("Y")-$y));
        return $age;
	}
	
}