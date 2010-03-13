<?php
class FrxDataEngine{

  public $conf; 
 
  public function __construct($conf, $repos_path) { 
    $this->conf = $conf; 
  }
  
  /**
   * Implements the basic default security check of calling 
   * an access method. 
   *
   * @param unknown_type $arg
   * @return unknown
   */
  public function access($arg) {
    $f = $conf['access callback']; 
    if ($F && is_callable($f)) { 
      return $f($arg); 
    } else { 
      return FALSE; 
    }
  }
  
}