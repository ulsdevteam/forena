<?php
// $Id$
require_once('forena.common.inc');
/**
 * @file
 * Class that defines default methods for access control in an FrxDataSource
 *
 */
class FrxDataSource{
  public $name;
  public $conf;
  public $block_path;
  public $comment_prefix;
  public $comment_suffix;
  public $block_ext;
  public $block_extensions;
  public $types;
  public $block_name;
  protected $te;
  public function __construct($conf, $repos_path, $name) {
    $this->conf = $conf;
    $this->comment_prefix = '--';
    $this->block_ext = 'sql';
    $this->block_path = $repos_path;
    $this->app = FrxReportGenerator::instance()->app;
    $this->block_extensions = array('inc', 'sql', 'xml');
    $this->name = $name;
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

  protected function loadBlockFromDB($block_name) {
    $block = array();
      // IF load from the database if forena_query is there.
    if (module_exists('forena_query')) {

      // See if we have one in the database first
      $sql = 'SELECT * FROM {forena_data_blocks} WHERE repository=:repos AND block_name = :block_name';
      $rs = db_query(
        $sql,
        array(
          ':repos' => $this->name,
          ':block_name' => $block_name,
        )
      );

      if ($b = $rs->fetchObject()) {
        $block = array(
          'repository' => $b->repository,
          'block_name' => $b->block_name,
          'type' => $b->block_type,
          'file' => $b->src,
          'access' => $b->access,
          'title' => $b->title,
          'locked' => $b->locked,
          'modified' => $b->modified,
        );
        switch ($block['type']) {
          case 'sql':
            $block = array_merge($block, $this->parseSQLFile($block['file']));
            break;
          case 'xml':
            $block = array_merge($block, $this->parseXMLFile($block['file']));
            break;
        }

      }
    }
    return $block;
  }

  protected function loadBlockFromFile($block_name) {
    $base_file = $this->block_path . '/' . $block_name;
    $parts = @explode('.', $block_name);
    $php_method = array_pop($parts);
    $php_class = implode('.', $parts);
    $php_file = $this->block_path . '/' . $php_class . '.inc';
    if (file_exists($base_file . '.sql')) {
      $contents = file_get_contents($base_file . '.sql');
      $block = $this->parseSQLFile($contents);
      $block['type'] = 'sql';
    }
    elseif (file_exists($base_file . '.xml')) {
      $contents = file_get_contents($base_file . '.xml');
      $block = $this->parseXMLFile($contents);
      $block['type'] = 'xml';
    }
    elseif (file_exists($php_file)) {
      require_once $php_file;
      if (class_exists($php_class)) {
        $o = new $php_class();
        $block['type'] = 'php';
        $block['access'] = @$o->access;
        $block['object'] = $o;
        $block['method'] = $php_method;
        if (method_exists($o, 'tokens' )) {
          $block['tokens'] = $o->tokens();
        }
        else {
          $block['tokens'] = array();
        }
      }
    }
    else {
      return array();
    }
    $block['locked']=1;
    return $block;
  }


  /**
   * Load blcok data from filesystem
   * @param $block_name
   */
  function loadBlock($block_name, $include=FALSE) {

    $block = array();
    if ($include) $this->block_name = $block_name;
    $block = $this->loadBlockFromDB($block_name);
    if (!$block) {
      $block = $this->loadBlockFromFile($block_name);
    }
    return $block;
  }


  /**
   * Load tokens from block source
   */
  public function tokens($source) {
    $tokens = array();
    // If we have a regular expression token parser, then get the tokens out of the block.
    if ($this->te) {
      $tokens = @$this->te->tokens($source);
      $tokens = array_diff($tokens, array('current_user'));
      //check tokens in the where clause
    }

    return $tokens;
  }

  public function listDBBlocks($search) {
    $data_blocks = array();
    $search = '%' . $search . '%';
    $sql = 'SELECT * from {forena_data_blocks} WHERE repository=:repos
      AND block_name like :search ';
    $rs = db_query($sql, array(':repos' => $this->name, ':search' => $search ));
    foreach ($rs as $block) {
      $data_blocks[] = $block->block_name;
    }
    return $data_blocks;
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
    $block_path .= $path . '*' . $search . '*';
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

      if (array_search($ext, $this->block_extensions)!==FALSE) {
        $block_list[] = str_replace($this->block_path . '/', '', $block_name);
      }
    }

    // Find directories
    $d = glob($path . '*');
    if ($d) foreach ($d as $dir_name) {
      if (is_dir($dir_name)) {
        $block_list =  array_merge($block_list, $this->list_blocks($search, $dir_name, $block_list));
      }
    }
    // Date
    if (!$subdir && module_exists('forena_query'))  {
      $block_list = array_merge($block_list,  $this->listDBBlocks($search));
      asort($block_list);
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
    return array('access' => $access, 'source' => $data, 'tokens' => $this->tokens($data));
  }

  public function getSQLInclude($block_name) {
    //@TODO: allow relative block includes
    $block = $this->loadBlock($block_name, TRUE);
    if ($block['type'] == 'sql') {
      return $block;
    }
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
    $source = '';
    $file = '';
    $skip = FALSE;
    $tokens = array();
    while ($i<$cnt) {
      $l = trim($lines[$i], "\r");
      @list($d, $c) = explode($comment, $l, 2);
      if ($trim) $c = trim($c, $trim);
      if  ($c) {
        @list($a, $o) = explode('=', $c, 2);
        $a = trim($a);
        if (($a && $o) || $c == 'END' || $c == 'ELSE' ) {
          switch ($a) {
            case 'ACCESS':
              $access = trim($o);
              break;
            case 'IF':
              $skip = !$this->te->test(trim($o));
              break;
            case 'END':
              $skip = FALSE;
              break;
            case 'ELSE':
              $skip = !$skip;
              break;
            case 'INCLUDE':
              $inc_block = $this->getSQLInclude(trim($o));
              if ($inc_block) {
                $data .= $inc_block['source'];
                array_merge($tokens, $inc_block['tokens']);
              }
              break;
          }

        }
        if ($a != 'ACCESS') $file .= "$l\n";

      }
      else {

        $file .= "$l\n";
      }
      if (!$skip) {
        if (strpos($l, $comment)!==0 && $l) {
          $data .= "$l\n";
        }
      }

      $i++;
    }
    $tokens = array_merge($tokens, $this->tokens($contents));
    $block = array( 'source' => $data, 'file' => trim($file, " \n"),
    	'tokens' => $tokens);
    if ($access) $block['access'] = $access;
    return $block;
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

  public function phpData($o, $method, $parameters = array()) {
    $data = NULL;
    if (is_object($o) && is_callable(array($o, $method))) {
      $data = $o->$method($parameters);
    }
    return $data;
  }

  /**
   * Save a data block
   * @param $block_name String name of block to save
   * @param $data array fields of block to save.
   */
  public function saveBlock($block_name, $data) {
    $modified = time();
    $rs = db_query('SELECT * from {forena_data_blocks} WHERE repository=:repos AND
      block_name = :block_name',
      array(
        ':repos' => $this->name,
        ':block_name' => $block_name,
        )
      );
    if ($block = $rs->fetchObject()) {
      db_update('forena_data_blocks')
        ->fields(array(
          'src' => $data['file'],
          'access' => $data['access'],
          'modified' => $modified,
        ))
        ->condition('repository', $this->name)
        ->condition('block_name', $block_name)
        ->execute();
    }
    else {
      db_insert('forena_data_blocks')
        ->fields( array(
          'repository' => $this->name,
          'block_name' => $block_name,
          'block_type' => $data['type'],
          'src' => $data['file'],
          'access' => $data['access'],
          'locked' => 0,
          'modified' => $modified,
        ))
        ->execute();

    }
  }
}

