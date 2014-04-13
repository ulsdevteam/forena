<?php
require_once 'FrxFile.inc';
require_once 'FrxReport.inc';
require_once 'forena.common.inc';
class FrxReportFile extends FrxFile {

  private $report_cache = array();
  /**
   * Constructor
   *   Sets the initial reort directory
   */
  public function __construct() {
    // Load default directory from configuration.
    $report_path = variable_get('forena_report_repos', '');
    if (!$report_path) {
      $report_path = variable_get('file_' . file_default_scheme() . '_path', conf_path() . '/files/reports');
    }
    $default_directory = rtrim($report_path, '/');
    $directories = module_invoke_all('forena_report_directory');
    foreach ($directories as $dir) {
      $this->includes[] = rtrim($dir, '/');
    }

    // Parent constructor.
    parent::__construct($default_directory, $directories, array('frx', 'skinfo', 'css', 'js'));
  }

  /**
   * List all the reports for a language.
   * @return unknown
   */
  public function allReports() {
    GLOBAL $language;
    $reports = array();
    $this->validateAllCache();
    $data = $this->getCache('frx');
    if ($data) foreach ($data as $base_name => $obj) {
      if ($obj->cache && $obj->cache['language'] == $language->language) {
        $obj->name = $base_name;
        $reports[ $obj->cache['category'] . $obj->cache['title']] = $obj;
      }
    }
    uasort($reports, 'FrxReportFile::reportCompare');
    return $reports;
  }

  /**
   * Sort compare function for sorting data by category then title.
   * @param unknown $a
   * @param unknown $b
   * @return number
   */
  static public function reportCompare($a, $b) {
    $c = strnatcasecmp($a->cache['category'], $b->cache['category']);
    if (!$c) {
      $c = strnatcasecmp($a->cache['title'], $b->cache['title']);
    }
    return $c;
  }
  /**
   * Get the cached information for a single report.
   * @param unknown $name
   * @return unknown
   */
  public function getReportCacheInfo($name) {
    GLOBAL $language;
    $this->validateAllCache();
    $data = $this->getCache('frx');
    if ($language->language != 'en') {
      $lang = $language->language;
      $name = "$lang/$name";
    }
    return @$data[$name];
  }

  public function menuReports() {
    GLOBAL $language;
    $this->validateAllCache();
    $data = $this->getCache('frx');

    $reports = array();
    foreach ($data as $base_name => $obj) {
      if ($obj->cache && $obj->cache['language'] == $language->language && isset($obj->cache['menu']['path'])) {
        $obj->name = $base_name;
        $reports[] = $obj;
      }
    }
    return $reports;
  }


  /**
   * Generate an ordered  list of reports by category
   * @param $categories
   * @return array Categories
   */
  public function reportsByCategory($categories = array()) {
    GLOBAL $language;
    $this->validateAllCache();
    $data = $this->getCache('frx');
    $reports = array();
    if ($data) foreach ($data as $base_name => $obj) {
      if ($obj->cache && @$obj->cache['category'] && !$obj->cache['hidden'] && $obj->cache['language'] == $language->language ) {
        $cache = $obj->cache;
        if (!$categories || array_search($cache['category'], $categories)!==FALSE) {
          // Check each callback function to see if we have an error.

          $access = FALSE;
          if (@$cache['access']) foreach ($cache['access'] as $provider => $callbacks) {
            if (user_access('access ' . $provider . ' data')) foreach ($callbacks as $callback=>$args) {
              if ($callback) foreach ($args as $arg) {
                if (function_exists($callback) && $arg) {
                  $a = $callback($arg);
                  if ($a) $access = TRUE;
                }
                else {
                  $access = TRUE;
                }
              }
              else {
                $access = TRUE;
              }
            }
          }
          if ($access) {
            $reports[$cache['category']][$cache['title']] = array(
                'title' => $cache['title'],
                'report_name' => $base_name,
            );
          }

        }
      }
    }
    $sort = defined('SORT_NATURAL') ? SORT_NATURAL : SORT_REGULAR;
    // Sort the reports
    if ($reports) foreach ($reports as $category => $list) {
      ksort($reports[$category], $sort);
    }
    ksort($reports, $sort);


    return $reports;
  }

  public function skins() {
    $this->validateAllCache();
    $this->getCache('skinfo');
    $skins = array();
    if (isset($this->cache['skinfo'])) foreach ($this->cache['skinfo'] as $name => $obj) {
      $skins[$name] = $obj->cache['name'];
    }
    return $skins;
  }

  /**
   * Should load cache data based on that.
   * @see FrxFile::buildCache()
   */
  public function buildCache($ext, $base_name, &$object) {
    switch ($ext) {
    	case 'frx':
    	  // Save language info
    	  $lang = 'en';
    	  if (module_exists('locale')) {
    	    @list($tlang, $tname) = explode('/', $base_name, 2);
    	    if (array_key_exists($tlang, language_list())) {
    	      $lang = $tlang;
    	    }
    	  }


    	  try {
    	    $r_xml =file_get_contents($object->file);
      	  // Load the report
        	 $r = @new FrxReport($r_xml);
    	  } catch (Exception $e) {
    	  }
        if (!$r->rpt_xml) {
    	    $s = t('Unable to load Report %s', array('%s' => $object->file));
    	    if (user_access('design any report')) {
    	      drupal_set_message($s, 'error', FALSE);
    	    }
        }

        // Get the security caches from the reports
        $cache = isset($r->rpt_xml) ? forena_load_cache($r->rpt_xml) : NULL;
        if ($r->rpt_xml) {
          $cache['title'] = $r->title;
          $cache['language'] = $lang;
          $cache['category'] = $r->category;
          $cache['hidden'] = @$r->options['hidden'];
          //$cache['hidden'] = $r->options['hidden'];
        }
        $object->cache = $cache;
    	  if ($r) $r->__destruct();
    	  unset($r);
    	  break;
    	case 'skinfo':
    	  $object->cache = drupal_parse_info_format(file_get_contents($object->file));
    	  break;
    }
  }

}
