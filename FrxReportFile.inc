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

  public function reportsByCategory() {
    $this->validateAllCache('frx');
    return $this->cache['frx'];
  }

  public function skins() {

    $this->validateAllCache('skinfo');
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
    	  //@TODO: Determine language based on file path.
    	  try {
    	    $r_xml =file_get_contents($object->file);
    	  } catch (Exception $e) {
    	    $s = t('unable to load Report %s', $r_xml);
    	    forena_error($s, $s . $e->getMessage());

    	  }

    	  // Load the report
    	  $r = new FrxReport($r_xml);
        // Get the security caches from the reports
        if ($r) $cache = $r ? forena_load_cache($r->rpt_xml) : 'report load error';
        $cache['title'] = $r->title;
        $cache['category'] = $r->category;
        $cache['hidden'] = @$r->options['cache']['hidden'];
        //$cache['hidden'] = $r->options['hidden'];
        $object->cache = $cache;
    	  $r->__destruct();
    	  unset($r);
    	  break;
    	case 'skinfo':
    	  $object->cache = drupal_parse_info_format(file_get_contents($object->file));
    	  break;
    }
  }

}
