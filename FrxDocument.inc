<?php
/** 
 * Given a report, render the appropriate output given the document format.
 * @author davidmetzler
 *
 */
class FrxDocument { 
  public $format; 
  
  
  public function loadCSSFIles($format) { 
    // Add css files
    $css = ''; 
    foreach (Frx::Skin()->stylesheets as $type=>$sheets) {
      foreach ($sheets as $sheet) { 
        switch ($type) { 
          case 'all': 
          case $format: 
            $css .= Frx::File()->contents($sheet); 
            break; 
        }
      }
    }
    return $css; 
  }

  // All document objects must implement this method. 
  public function render($r, $format, $options = array()) { 
    $header = '<h1>' . $r->title . '</h1>';
    $output = $header . $output;
    $css_text = '';
    $r_text = '';
    
    $stylesheets = Frx::Skin()->stylesheets;
    foreach ($stylesheets as $type=>$sheets) { 
      foreach($sheets as $sheet) { 
        switch ($type) { 
          case 'all': 
          case $this->format:
            $css_file[] = $sheet; 
            break;  
        }
      }
    }
    
    if ($css_files) foreach ($css_files as $css_file) {
      $css_text .= file_get_contents($css_file);
    }
    $options = array('css' => $css_text,
                     'docname' => str_replace(' ', '_', $title),
                     'xml' => $r_text,
                     'title' => $title,
               );
  
    $output = $this->generate_doc($format, $output, $options, $print);
    if ($format!='email') {
      print $output;
    }
    
  }
}