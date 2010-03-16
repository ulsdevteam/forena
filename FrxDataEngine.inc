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
    $f = $this->conf['access callback']; 
    if ($f && is_callable($f)) {
      return $f($arg); 
    } else { 
      return user_access('access content'); 
    }
  }
  
}