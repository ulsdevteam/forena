<?php
// $Id$
require_once('forena.common.inc');
/**
 * @file
 * Class that defines default methods for access control in an FrxDataSource
 *
 */
class FrxDataSource{

  public $conf;
  public $block_path;
  public $comment_prefix;
  public $comment_suffix;
  public $block_ext;
  public $types; 
  protected $te;
  public function __construct($conf, $repos_path) {
    $this->conf = $conf;
    $this->comment_prefix = '--';
    $this->block_ext = 'sql';
    $this->block_path = $repos_path;
    $this->app = FrxReportGenerator::instance()->app;
  }

  /**
   * Implements the basic default security check of calling
   * an access method.
   *
   * @param unknown_type $arg
   * @return unknown
   */
  public function access($arg) {
    $f = @$this->conf['access callback'];
    if ($f && is_callable($f)) {
      return $f($arg);
    }
    elseif (isset($this->conf['access block'])) {
      $block = @$this->conf['access block'];
      $path='';
      if (isset($this->conf['access path'])) $path = $this->conf['access path'];
      return FrxReportGenerator::instance()->block_access($block, $path, $arg);
    }
    else {
      return user_access('access content');
    }
  }
  
  /**
   * Load blcok data from filesystem
   * @param $block_name
   */
  function loadBlock($block_name) {
    $block = array(); 
    $base_file = $this->block_path . '/' . $block_name; 
    if (file_exists($base_file . '.sql')) { 
      $contents = file_get_contents($base_file . '.sql'); 
      $block = $this->parseSQLFile($contents); 
      $block['type'] = 'sql'; 
      $block['tokens'] = $this->tokens($block['source']); 
    }
    elseif (file_exists($base_file . '.xml')) { 
      $contents = file_get_contents($base_file . '.xml'); 
      $block = $this->parseXMLFile($contents); 
      $block['type'] = 'xml'; 
      $block['tokens'] = $this->tokens($block['source']); 
    }
    elseif (file_exists($base_file . '.inc')) { 
      $parts = pathinfo($base_file . '.inc'); 
      @list($class, $method) = explode('.', $parts['filename']);
      $inc = $base_file . '.inc';
      require_once $inc;
      if (class_exists($class)) { 
        $o = new $class(); 
        $block['type'] = 'php'; 
        $block['access'] = @$o->access; 
        $block['object'] = $o;
        $block['method'] = $method; 
        if (method_exists($o, 'tokens' )) { 
          $block['tokens'] = $o->tokens(); 
        } 
      }
    } 
    return $block; 
  }


  /**
   * Load tokens from block source
   */
  public function tokens($soucre) {
    $tokens = array(); 
    // If we have a regular expression token parser, then get the tokens out of the block.
    if ($this->te) {
      $tokens = @$this->te->tokens($source);
      $tokens = array_diff($tokens, array('current_user'));
      //check tokens in the where clause
    }

    return $tokens;
  }

  /**
   * Find all the blocks matching a provided search string
   *
   * @param string $search part block names to search for
   * @return unknown
   */
  public function list_blocks($search, $subdir='', $this_list=array()) {
    $block_list = $this_list;
    // First find files that match the search string
    $path = $this->block_path . '/';
    if ($subdir) $path = $subdir . '/';
    $block_path .= $path . '*' . $search . '*.' . $this->block_ext;

    // Find sql files
    $d = glob($block_path);
    if ($d) foreach ($d as $file_name) {
    	// Split off the extention
    	$p = strripos($file_name, '.');
      if ($p!==FALSE) {
  	    $ext = substr($file_name, $p+1);
  	    $block_name = substr($file_name, 0, $p);
      }
      else {
      	$ext = '';
      	$block_name = $file_name;
      }
    	$block_list[] = str_replace($this->block_path . '/', '', $block_name);
    }

    // Find directories
    $d = glob($path . '*');
    if ($d) foreach ($d as $dir_name) {
      if (is_dir($dir_name)) {
        $block_list += $this->list_blocks($search, $dir_name, $block_list);
      }
    }

    return $block_list;
  }

  public function debug($msg='', $log='') {
  	FrxReportGenerator::instance()->app->debug($msg, $log);
  }

  public function error($msg='', $log='') {
  	FrxReportGenerator::instance()->app->error($msg, $log);
  }

  public function parseXMLFile($contents) {
  	$comment = $this->comment_prefix;
  	$trim = $this->comment_suffix;
    $lines = explode("\n", $contents);
    $cnt = count($lines);
    $access = '';
    $i=0;
    $block = '';
    $data = '';
    while ($i<$cnt) {
      $l = trim($lines[$i], "\r");
      @list($d, $c) = explode($comment, $l, 2);
      if ($trim) $c = trim($c, $trim);
      if  ($c) {
        list($a, $o) = explode('=', $c, 2);
        $a = trim($a);
        if ($a && $o) {
	        switch ($a) {
	        	case 'ACCESS':
	        		$access = trim($o);
	        		break;
	        	default:


        }

        }

      }
      if (strpos($l, $comment)!==0) {
        $data .= "$l\n";
      }
      $i++;
    }
    return array('access' => $access, 'source' => $data);
  }

  public function parseSQLFile($contents) {
  	$comment = $this->comment_prefix;
  	$trim = $this->comment_suffix;
    $lines = explode("\n", $contents);
    $cnt = count($lines);
    $access = '';
    $i=0;
    $block = '';
    $parms = Frx::Data()->currentContext();
    $data = '';
    while ($i<$cnt) {
      $l = trim($lines[$i], "\r");
      @list($d, $c) = explode($comment, $l, 2);
      if ($trim) $c = trim($c, $trim);
      if  ($c) {
        list($a, $o) = explode('=', $c, 2);
        $a = trim($a);
        if ($a && $o) {
	        switch ($a) {
	        	case 'ACCESS':
	        		$access = trim($o);
	        		break;
	        	default:

        }

        }

      }
      if (strpos($l, $comment)!==0 && $l) {
        $data .= "$l\n";
      }
      $i++;
    }


    return array('access' => $access, 'source' => $data);
  }
  
  /**
   * Dummy method for returning sql data
   * @param unknown_type $parameters
   */
  public function sqlData($parameters) { 
    return ''; 
  }

  /**
   * Implement static XML functioin 
   * @param $source XML Source data from block load
   * @param $parm_data Parameter data
   */
  public function xmlData($source, $parm_data='') {
    $xml ='';
    try {
      $xmlData = $source;
      $xml = new SimpleXMLElement($xmlData);
    } catch (Exception $e) {
      $this->error("Error processing xml\n", $e->getMessage() . "\n" . $xmlData);
    }
    return $xml;
  }
  
  public function phpData($method, $paremeters) { 
    $xml = ''; 
    return $xml; 
  }
}

