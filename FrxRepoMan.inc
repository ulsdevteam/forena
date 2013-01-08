<?php
/**
 *
 * Enter description here ...
 * @author davidmetzler
 *
 */
class FrxRepoMan {

  public $repositories;


  /**
   * Loads all of the include files that
   */
  private function define_plugins( $class='') {
    static $plugins = '';
    if (!$plugins) {
      $plugins = array();
      foreach (module_list() as $module) {
        $function = $module . '_forena_plugins';
        if (function_exists($function)) {
          $returned_plugins = $function();
          if ($returned_plugins) foreach ((array)$returned_plugins as $p) {
            $p['module'] = $module;
            $p['file'] = drupal_get_path('module' , $p['module']) . '/' . $p['file'];
            $plugins[]=$p;
          }
        }
      }
    }

    foreach ($plugins as $p) {
      if (($class=='' || $class==$p['class'])) {
        if ($p['file'] ) {
          include_once( trim($p['file'], '/'));
        }
      }
    }
  }

  //Determine data sources.
  public function __construct() {
    global $_forena_repositories;
    // Empty repository so we need to initialize
    // Build the default sample one
    $repos = array();

    // Load the repository list from the global settings.php file.
    if ($_forena_repositories) {
      $repos = $_forena_repositories;
    }

    // Overide difinitions of the sample and drupal repositories.
    $path = drupal_get_path('module', 'forena');
    $repos['forena_help'] = array(
      'path' => $path .'/repos/forena_help',
      'title' => 'Forena Help Reports',
    );

    $repos['drupal'] = array(
      'path' => $path . '/repos/drupal',
      'title' => 'Drupal Reports',
    );

    $repos['sampledb'] = array(
      'path' => $path . '/repos/sample',
      'title' => 'Sample DB Repository'
    );

    // Retrieve the repositories defined in the database;
    $results = db_query('SELECT * FROM {forena_repositories}');
    foreach ($results as $r) {
    	if ($r->config) {
    		$new_r = unserialize($r->config);
    	}
    	else {
    		$new_r = array();
    	}
    	$r_name = $r->repository;
    	if (is_array(@$repos[$r_name])) {
    		$new_r = array_merge($new_r, $repos[$r_name]);
    	}
    	else {
    		$new_r['source'] = 'user';
    	}
    	$new_r ['title'] = $r->title;
    	$new_r ['enabled'] = $r->enabled;

    	$repos[$r_name] = $new_r;
    }

    drupal_alter('forena_repos',  $repos);

    if ($_forena_repositories) {
    	array_merge($repos, $_forena_repositories);
    }

    $this->repositories = $repos;

  }

  /**
   * Load repository
   * @param $name Name of the repository
   */
  public function repository($name) {
    // Now determine if the object exists
    if (isset($this->repositories[$name])) {
      if (@!is_object($this->repositories[$name]['data'])) {
        $this->load_repository($this->repositories[$name]);
      }
      return $this->repositories[$name]['data'];
    }
    else {
      Frx::error('Undefined repository' . $name, "Undefined Repository: $name ");
    }
  }

  // Putting this in a function to sandbox the repository settings
  public function load_repository(&$repo) {
    // First determine if the class file exisits
    $path = @$repo['path'];
    $conf = array();

    if (file_exists($path . '/settings.php')) {
      // Override with repository specific data
      include($path . '/settings.php');
    }

    $repo = array_merge($conf, $repo);
    if (!isset($repos['data'])||!is_object($repo['data'])) $repo['data'] = $this->load_provider($repo, $path);

  }

  /**
   * Load the data provider class based on the class name.
   *
   * @param string $name
   * @return object The data provider object
   */
  public function load_provider($conf, $repo_path) {
    @$name = isset($conf['data provider'])? $conf['data provider'] : $conf['data_engine'];
    $this->define_plugins();
    // Instantiate the path
    if (class_exists($name)) {
      $o = new $name($conf, $repo_path);
      return $o;
    }
    else {
      Frx::error('Data provider not found for '. $conf['title']);
    }
  }


  public function loadBlock($data_block) {
    $block = array();
    list($provider, $block_name) = explode('/', $data_block, 2);
    $repos = @$this->repositories[$provider];
    if (isset($repos['enabled']) && !$repos['enabled']) {
     return array();
    }

    $o = $this->repository($provider);
    if ($o) {
     $access = TRUE;
     $block = $o->loadBlock($block_name);
    }

    return $block;
  }


  /**
   * Extract the data by running a block
   *
   * @param unknown_type $data_block
   * @param unknown_type $parameters
   * @param string $clause order / where cause
   * @return unknown
   */
  function data($data_block) {
     list($provider, $block_name) = explode('/', $data_block, 2);
     //Intstantiate the provider
     $o = $this->repository($provider);
     $repos = @$this->repositories[$provider];


     if (isset($repos['enabled']) && !$repos['enabled']) {
     	return '';
     }
     //Populate user callback.
     if (isset($repos['user callback'])) {
       $user_fn = $repos['user callback'];
       if (is_callable($user_fn)) {
       	 $current_user =   $user_fn();
         Frx::Data()->setValue('current_user', $current_user);
       }
     }


     $xml = '';
     if ($o) {
       $access = TRUE;
       $block = $o->loadBlock($block_name);
       $right = @$block['access'];
       if ($block && $o->access($right)) {
         switch ($block['type'])  {
           case 'sql':
             $xml = $o->sqlData($block['source']);
             break;
           case 'xml':
             $xml = $o->xmlData($block['source']);
             break;
           case 'class':
             $xml = $o->phpData($o, $block['method']);
             break;
         }

       }
       return $xml;
     }
     else {
       return '';
     }
  }

  /**
   * Check access control using the block in a data block.  In this case
   * public assess returns true.
   * @param $block  Repository block used to test access
   * @param $path   xpath to user right within xml data.
   * @param $access Access to test
   * @return boolean
   */
  function blockAccess($block, $path, $access, $cache=TRUE) {
  	// PUBLIC always returns true.
    if ($access=='PUBLIC') return TRUE;
    if (!isset($_SESSION['forena_access'])) $_SESSION['forena_access'] = array();
    if ($cache && isset($_SESSION['forena_access'][$block])) {
      $rights = $_SESSION['forena_access'][$block];
    }
    else {
      $rights = array();
      // Get the block from the repository
      $frxData = Frx::Data();
      $frxData->push(array(), 'frx-block-access');
      $data = Frx::RepoMan()->data($block);
      $frxData->pop();
      if ($data) {
        if (!$path) {
          $path ='*/*';
        }
        $nodes = $data->xpath($path);
        if ($nodes) foreach ($nodes as $node) {
          $rights[] = (string)$node;
        }
        $_SESSION['forena_access'][$block]=$rights;
      }
    }
    foreach ($rights as $right) {
      if ($access == $right) return TRUE;
    }
    return FALSE;
  }


/*
 * Recieves a datablock and returns an array of values from the data block.
 * @data_block: name of the data block to be invoked for values
 * @field: Specific field name within the data block. The values returned will only
 * come from this field.
 * @params: filtering for the data block
 * @where_clause: Where clause for data block to be filtered against.
 *
 */
function dataBlockParams($data_block, $field, $label, $params=array()) {
	Frx::Data()->push($params);
  $xml = Frx::RepoMan()->data($data_block);
  Frx::Data()->pop();

  $list = array();
  if ($xml) {
    $path = ($field) ? $field : '*[1]';
    $label_path = ($label) ? $label : '*[2]';


    //walk through the $xml.
    $rows = $xml->xpath('*');
    if ($rows) foreach ($rows as $row) {
    	$value = $row->xpath($path);
    	$label_field = $row->xpath($label_path);
    	$label_value =  $label_field ? (string)$label_field[0] : (string)$value[0];
    	$list[(string)$value[0]] = (string)$label_value;

    }
  }
  return $list;
}

}