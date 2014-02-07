<?php
require_once 'FrxFile.inc';
require_once 'forena.common.inc';
class FrxDataFile extends FrxFile {

  private $report_cache = array();
  /**
   * Constructor
   *   Sets the initial reort directory
   */
  public function __construct() {
    // Load default directory from configuration.
    $report_path = variable_get('forena_query_data_path', '');
    if (!$report_path) {
      $report_path = variable_get('file_private_path', conf_path() . '/data');
    }

    // Add directories for data  repos.
    $default_directory = rtrim($report_path, '/');
    $repositories = Frx::RepoMan()->repositories;
    $directories = array();
    foreach ($repositories as $k => $repos) {
      $directories[$k] = $repos['path'];
    }
    foreach ($directories as $dir) {
      $this->includes[$k] = rtrim($dir, '/');
    }

    // Parent constructor.
    parent::__construct($default_directory, $directories, array('sql', 'xml', 'inc'));
  }

  /**
   * List all the reports for a language.
   * @return unknown
   */
  public function userBlocks($search = '*') {
    $blocks = array();
    $sql = array_values($this->validateAllCache('sql'));
    $inc = array_values($this->validateAllCache('inc'));
    $xml = array_values($this->validateAllCache('xml'));
    $data =$sql + $inc + $xml;
    if ($data) foreach ($data as $base_name => $obj) {
  	  if ($search == '*' || drupal_match_path($base_name, $search)) {
        if ($obj->cache) {
    	    $r = Frx::RepoMan()->repository($provider);
    	    if ($r && $r->access($obj->access)) {
            $obj->name = $base_name;
            $blocks[$obj->cache['name']] = $obj;
    	    }
        }
  	  }
    }
    uasort($blocks, 'FrxDataFile::blockCompare');
    return $blocks;
  }


  public function scan($prefix = '') {
    // Add the base report files.
    $this->scanInclude($this->dir, '');

    // Now add the module provided ones.
    if ($this->includes) foreach ($this->includes as $repos => $directory) {
      $this->scanInclude($directory, $repos . '/');
    }
    $this->needScan = FALSE;
  }
  /**
   * Sort compare function for sorting data by category then title.
   * @param unknown $a
   * @param unknown $b
   * @return number
   */
  static public function blockCompare($a, $b) {
    $c = strnatcasecmp($a->cache['provider'], $b->cache['name']);
    if (!$c) {
      $c = strnatcasecmp($a->cache['provider'], $b->cache['name']);
    }
    return $c;
  }

  /**
   * Should load cache data based on that.
   * @see FrxFile::buildCache()
   */
  public function buildCache($ext, $base_name, &$object) {
    $block_name = $base_name;
    $parts = explode('/', $base_name);
    $provider = array_shift($parts);
    $block = Frx::RepoMan()->loadBlock($block_name);
    $object->cache = array(
    	'provider' => $provider,
      'name' => $block_name,
      'access' => @$block['access'],
      'type' => @$block['type'],
      'options' => @$block['options'],
    );
  }

}
