<?php
// $Id$
/**
 * @file
 * Provides data blocks for native drupal connections using the default 
 * drupal connections.
 *
 */
class FrxControls {  
  /**
   * @section 
   * Formatters 
   * Below here are formatting types
   */
  public function formats() {
    $formats = array('drupal_date_format' => 'Drupal Date',
                     ); 
    return $formats; 
  }
  
  public function doc_types() {
    $doc_types = array('doc' => 'htmldocument',
                       'html' => 'htmldocument',
    );
    return $doc_types;
  }
  

  public function drupal_date_format($value, $format_str) { 
     switch ($format_str) { 
       case 'medium': 
       case 'small':
       case 'large': 
         $type = $format_str;
         $format=''; 
         break; 
       default: 
         $type = 'custom'; 
         $format = $format_str; 
     }     
     return format_date($value, $type, $format);     
  } 

  public function htmldocument($body, $options) {
    $output .= '<html><head>';
    if ($options['css'] && file_exists($options['css'])) {
      $css = file_get_contents($options['css']);
      $output .= '<style type="text/css">';
      $output .= $css;
      $output .= '</style>';
    }
    $output .= '<title>'. $options['title'] .'</title></head>'. $body .'</html>';
    return $output;
  } 
}