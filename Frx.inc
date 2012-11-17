<?php
/**
 * General Forena Reporting Class
 */
class Frx { 
  
  /**
   * Skin Factory
   * @return FrxSkin
   */
  public static function Skin() { 
    static $o = ''; 
    if (!$o) { 
      require_once 'FrxSkin.inc';
      $o = new FrxSkin(); 
    }
    return $o; 
  }
  
  /**
   * Data Factory
   */
  public static function Data() {
    static $o = ''; 
    if (!$o) { 
      require_once 'FrxData.inc'; 
      $o = new FrxData(); 
    }
    return $o; 
  }
  
}