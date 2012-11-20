<?php
/**
 * Skinning 
 * 
 * A skin is a collectio of css and js files that need to get used by 
 * an application or reports.  Skins are idntified by .info files contained
 * in the report directory.  
 * 
 * Skins can specify external JS Libraries as well as 
 * 
 * Used to be called a "form"
 */

class FrxSkin { 
  public $name = ''; 
  public $stylesheets = array(); 
  public $scripts = array(); 
  public $default_skin = ''; 
  public $info = ''; 
  
  /**
   * Method constructor. 
   */
  public function __construct() { 
    $this->default_skin = variable_get('forena_default_form', 'default_skin');    
  }
  
  /**
   * Add CSS Files
   * @param $type
   * @param $sheet
   */
  public function addCSS($type, $sheet) { 
    if (Frx::File()->exists($sheet)) { 
      $stylesheets[$type][] = Frx::File()->path($sheet);  
    }
    elseif (file_exists($sheet)) { 
      $stylesheets[$type][] = $sheet; 
    }
  }
  
  /**
   * 
   * @param unknown_type $script
   */
  public function addJS($script) { 
    if (Frx::File()->exists($script)) { 
      $scripts[] = Frx::File()->path($script); 
    } 
    elseif (file_exists('sites/all/libraries' . $script)) { 
      $scripts[] = 'sites/all/libraries' . $script; 
    }
    elseif (file_exists($script)) { 
      $scripts[] = $script; 
    }
  }
  
  /**
   * Load the skin based on the name. 
   * @param $name path/name of skin.  
   */
  public function load($name) { 
    if (!$name) $name = $this->default_skin; 
    if ($this->name != $name) {
      
      //Check for an info file
      if (Frx::File()->exists($name . '.info')) { 
        
        // Parse the info file using drupals stock version
        $this->info = drupal_parse_info_format(Frx::File()->contents($name . '.info')); 
        if (isset($this->info['stylesheets'])) { 
          foreach($this->info['stylesheets'] as $type => $sheets) { 
            foreach($sheets as $sheet) { 
              $this->addCSS($type, $sheet); 
            }
          }
        }
        
        // Add the javascript files
        if (isset($this->info['scripts'])) foreach ($scripts as $script) { 
          $this->addJS($script); 
        }
        
      }
      else { 
        $this->info = array(
          'name' => $name,
        ); 
        //Perform the legacy detection of css and pdf files. 
        $this->addCSS('all', $name . '.css'); 
        $this->addJS($name . '.js'); 
      }
      
      $this->name = $name; 
      
    }
    return $this; 
  }
  
}