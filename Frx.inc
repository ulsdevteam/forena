<?php
/**
 * General Forena Reporting Class
 */
define('FRX_TOKEN_EXP', '/\{[^\n^\r^}]+}/');
define('FRX_SQL_TOKEN', '/:([a-z]|[0-9]|{A-Z]|[_\.])+/');

class Frx { 
  
  /**
   * Skin Factory
   * @return FrxSkin
   */
  public static function Skin() { 
    static $o = ''; 
    if (!$o) { 
      require_once 'FrxSkin.inc';
      $o = new FrxSkin(); 
    }
    return $o; 
  }
  
  /**
   * Data Factory
   */
  public static function Data() {
    static $o = ''; 
    if (!$o) { 
      require_once 'FrxData.inc'; 
      $o = new FrxData(); 
    }
    return $o; 
  }
  
  /**
   * File singleton factory
   */
  public static function File() {
    static $o = ''; 
    if (!$o) { 
      require_once 'FrxFile.inc';
      $o = new FrxFile(); 
    }
    return $o; 
  }

  /**
   * File singleton factory
   * @return FrxMenu
   */
  public static function Menu() {
    static $o = ''; 
    if (!$o) { 
      require_once 'FrxMenu.inc';
      $o = new FrxMenu(); 
    }
    return $o; 
  }
  /**
   * 
   * Enter description here ...
   * @param unknown_type $type
   * @return FrxDocument
   */
  public static function Document($type='web') {
    
    static $doc_types = ''; 
    static $objects = ''; 
    
    // Invoke doc_type hook to see which document types are there. 
    if (!$doc_types) $doc_types = module_invoke_all('forena_document_types'); 
        
    if (!$type) $type = 'web';
    if (isset($doc_types[$type]) && class_exists(@$doc_types[$type]['class'])) { 
      if (!@$doc_types[$type]['object']) { 
        $o = new $doc_types[$type]['class']; 
        $o->format = $type; 
        $doc_types[$type]['object'] = $o; 
      }
      else { 
        $o = $doc_types[$type]['object']; 
      }
      return $o; 
    } 
    else { 
      drupal_set_message('Unsupported document type:  '. $type, 'error'); 
    }
    
  }
  
  public static function Controls( $class='') {
    static $instances = '';
    static $classes = ''; 
    if (!$instances) {
      $classes = FrxReportGenerator::instance()->app->controls();
      foreach ($classes as $c) {
        if ($c['file']) {
          include_once($c['file'] );
          if (class_exists($c['class'])) {
            $instances[$c['class']] =  new $c['class'];
          }
        }
      }
    }

    if ($class) {
      return @$instances[$class];
    }
    return $instances;
  }
  
  /**
   * Forena Repository manager 
   * Class factory
   * @return FrxRepoMan
   */
  public static function RepoMan() { 
    static $o = ''; 
    if (!$o) { 
      require_once 'FrxRepoMan.inc';
      $o = new FrxRepoMan(); 
    }
    return $o;
  }
  
  /**
   * General wrapper procedure for reporting erros
   *
   * @param string $short_message Message that will be displayed to the users
   * @param string $log Message that will be recorded in the logs.
   */
  public static function error($short_message='', $log='') {
    if ($short_message) {
      drupal_set_message(check_markup($short_message), 'error');
    }
    if ($log) {
      watchdog('forena', $log, NULL, WATCHDOG_ERROR);
    }
  }
  
  /**
   * Debug handler
   * Enter description here ...
   * @param unknown_type $short_message
   * @param unknown_type $log
   */
  public static function debug($short_message='', $log='') {
    if ($log) {
      watchdog('forena debug', $log, NULL);
    }
    if ($short_message) {
      drupal_set_message(check_markup($short_message));
    }
  }
  
  public static function SyntaxEngine($regexp, $trim) { 
    require_once 'FrxSyntaxEngine.inc';
    return new FrxSyntaxEngine($regexp, $trim); 
  }
  
}