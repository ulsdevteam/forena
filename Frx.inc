<?php
/**
 * General Forena Reporting Class
 */
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
  
}