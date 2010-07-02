<?php
// $Id$
require_once('forena.common.inc'); 
/**
 * @file
 * Class that defines default methods for access control in an FrxDataEngine
 *
 */
class FrxDataProvider{

  public $conf; 
  public $block_path; 
  public $comment_prefix; 
  public $comment_suffix; 
  public $block_ext; 
  protected $te; 
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
  
 
  /**
   * Default block load
   * Loads the data block based on the block name from the file system.  The classes that 
   * are derived from this will set the block_ext property, which in most cases is .sql but 
   * might be something different.   The load of the block file should return data, access and
   * probably parameters to. 
   * @param unknown_type $block_name
   * @return unknown
   */
  public function load_block($block_name) { 
    $filename = $this->block_path .'/'. $block_name .'.'. $this->block_ext;  
    $block = forena_load_block_file($filename, $this->comment_prefix, $this->comment_suffix);
    // If we have a regular expression token parser, then get the tokens out of the block.  
    if ($this->te) { 
      $tokens = $this->te->tokens($block['source']); 
      $block['tokens'] = $tokens; 
    }

    return $block; 
  }
  
  /**
   * Find all the blocks matching a provided search string
   *
   * @param string $search part block names to search for
   * @return unknown
   */
  public function list_blocks($search) { 
    $block_list = array(); 
    
    // First find files that match the search string
    $path = $this->block_path. '/*'. $search . '*.' . $this->block_ext;
    $d = glob($path); 
    foreach ($d as $file_name) {
      list($block_name, $ext) = explode('.', $file_name);  
      $block_list[] = str_replace($this->block_path .'/','',$block_name);
    }         
    return $block_list; 
  }
}