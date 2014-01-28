<?php

/**
 *	@file: Binary.inc	
 *	@author: Soyoes 2011/08/18
 *	@uses:
 *	@example: 
 *****************************************************************************/

class Binary {
    var $data; // inputdata
    var $byteOffset;
    var $bitOffset;

    static function read($data){
    	$b = new Binary();
    	$b->data = $data;
    	$b->byteOffset = 0;
        $b->bitOffset = 0;
        return $b;
    }
    
    /** move cursor */
    function resetBitCursor($n=1) {
        if ($this->bitOffset > 0) {
            $this->byteOffset += $n;
            $this->bitOffset = 0;
        }
    }
    
    // step n bit
    function step($bit=1){
    	$this->bitOffset += $bit;
        if (8 <= $this->bitOffset) {
            $this->byteOffset += (int) $bit/8;
            $this->bitOffset = $bit % 8;
        }else if(0>$this->bitOffset){
        	$this->bitOffset += 8;
        	$this->byteOffset -= (int) $bit/8;
        }
    }
    
    function hasNext(){
    	return ($this->byteOffset < strlen($this->data));
    }
    
    function moveCursor($byte, $bit){
    	$this->byteOffset = $byte;
    	$this->bitOffset = $bit;
    }
    
    function getData($length) {
    	if ($length == 0)
    		return $this->data;
        $this->resetBitCursor();
        $data = substr($this->data, $this->byteOffset, $length);
        $data_len = strlen($data);
        $this->byteOffset += $data_len;
        return $data;
    }
    
    function getUI8() {
        $this->resetBitCursor();
        $value = ord($this->data{$this->byteOffset});
        $this->byteOffset += 1;
        return $value;
    }
    function getUI16LE() {
        $this->resetBitCursor();
        $ret = unpack('v', substr($this->data, $this->byteOffset, 2));
        $this->byteOffset += 2;
        return $ret[1];
    }
    function getUI32LE() {
        $this->resetBitCursor();
        $ret = unpack('V', substr($this->data, $this->byteOffset, 4));
        $this->byteOffset += 4;
        return $ret[1];
    }
    
    /**
     * return ord int. eg.12
     * */
    function getUIBit() {
        $value = ord($this->data{$this->byteOffset});
        $value = 1 & ($value >> (7 - $this->bitOffset));
        $this->bitOffset ++;
        if (8 <= $this->bitOffset) {
            $this->byteOffset++;
            $this->bitOffset = 0;
        }
        return $value;
    }
    
    /**
     * S: signed value @see adobe spec P:15 about SB & UB
     * SB: shift right get double length , if its value == 1111...1 (max)
     * use to specify length 
     * */
    function getSBits($width){
    	$s = $this->getBits($width);
    	$this->step(-1*$width);
    	if(!str_has($s, "0")){
    		$this->getUIBits($width);
    		$width *= 2;
    	} 
    	return $this->getUIBits($width);
    }
    
    /**
     * return ord int eg.6577
     * */
    function getUIBits($width) {
        $value = 0;
        for ($i = 0 ; $i < $width ; $i++) {
            $value <<= 1;
            $value |= $this->getUIBit();
        }
        return $value;
    }
    
    /**
     * return str eg."10011"
     * */
    function getBits($width){
    	$value = "";
        for ($i = 0 ; $i < $width ; $i++) {
            $value .= $this->getUIBit(); //$this->data{$i};
        }
        return $value;
    }
    
    function getHex($length){
    	$data = $this->getData($length);
    	return self::str2Hex($data);
    }
    
    function getEndian($length){
    	$d = $this->getData($length);
    	return self::str2Endian($d);
    }
    
	function getEndianHex($length){
    	$d = $this->getData($length);
    	return self::str2EndianHex($d);
    }
    
    function getASCII($length){
    	$d = $this->getData($length);
    	return ord($d);
    }
    
    static function str2EndianHex($str){
    	$hex = self::str2Hex($str);
    	$val = "";
    	for ($i = strlen($hex); $i>=2; $i-=2){
    		$val .= substr($hex, $i-2, 2);
    	}
    	return $val;
    } 
    
    static function str2Endian($str){
    	$hex = self::str2Hex($str);
    	$val = 0;
    	$sq = strlen($hex) / 2 - 1;
    	for ($i = strlen($hex); $i>=2; $i-=2){
    		$v = hexdec(substr($hex, $i-2, 2));
    		if ($v > 0){
    			$val += $v * pow(256, $sq);
    		}
    		$sq -= 1;
    	}
    	return $val;
    } 
    
    static function str2Hex($string){
    	$hexstr = unpack('H*', $string);
  		return array_shift($hexstr);
	}
	
	static function hex2Str($hex){
   		return pack('H*', $hex);
	}
}