<?php
// $Id$
/**
 * @file
 * Class that defines default methods for access control in an FrxDataEngine
 *
 */
class FrxDataEngine{

  public $conf; 
  public $block_path; 
  public $comment_prefix; 
  public  $comment_suffix; 
  public $block_ext; 
 
  public function __construct($conf, $repos_path) { 
    $this->conf = $conf;
    $this->comment_prefix = '--';  
    $this->block_ext = 'sql';
    $this->block_path = $repos_path;  
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
    } 
    else { 
      return user_access('access content'); 
    }
  }
  
  public function load_block($block_name) { 
    $filename = $this->block_path .'/'. $block_name .'.'. $this->block_ext;  
    $block = forena_load_block_file($filename, $this->comment_prefix, $this->comment_suffix); 
    return $block; 
  }
  
}