<?php
/**
 * @file FrxRenderer.inc
 * Base class for Frx custom renderers
 * @author davidmetzler
 *
 */
class FrxRenderer {
  public $teng;           // Token replacement engine.
  public $reportDocDomNode; //A Dom node version of the element.  This is important if you want to walk text nodes.
  public $reportDocNode;  // SimpleXML Report Document Node -- The node of the report we are rendering
  public $frxAttributes;  // Frx Attributes of the node we are rendering.
  public $htmlAttributes;  // Html attributes of the node that we are rendering
  public $dataProvider;   // An FrxData instance that provides the data assiated with the report.
  public $name;
  public $id;
  public $format;
  public $frxReport;  // The report object being used.


  public function __construct($domNode = NULL, $teng = NULL, $frxReport = NULL) {
    if ($domNode) {
      $this->initReportNode($domNode, $teng);
    }
  }

  public function initReportNode($domNode, $frxReport, $format='web') {
    $this->format= $format;
    $this->reportDocDomNode = $domNode;
    $this->dataProvider = Frx::Data();
    $this->reportDocNode = simplexml_import_dom($domNode);
    $node = $this->reportDocNode;
    $this->name = $node->getName();
    $this->htmlAttributes = $node->attributes();
    $this->id = (string)$this->htmlAttributes['id'];
    $this->frxAttributes = $node->attributes(FRX_NS);
    $this->teng = $frxReport->teng;
    $this->frxReport = $frxReport;
  }

  public function replaceTokens($text, $raw_mode=FALSE) {
    return $this->teng->replace($text, $raw_mode);
  }

  public function render() {
    // Get the data that we're working with
    $xml = $this->dataProvider->getCurrentContext();
    $node = $this->nodes;
    // We can render so lets do it.
    $text = $this->xmlNode->asXML();
    $o = $this->teng->replace($text);
    return $o;
  }

  public static function addAttributes(&$attributes, $key, $value) {
    $parts = explode('_', $key);
    $suff = '';
    if (count($parts) > 1) {
      $suff=array_pop($parts);
      $part = implode('_', $parts);
    }

    // If we have _0 _1 _2 attributes convert them into arrays.
    if ((int)$suff || $suff === '0') {
      $attributes[$part][] = (string)$value;
    }
    else {
      $attributes[$key] = (string)$value;
    }
  }

  /**
   * Standard php array containing merged attributes
   * Enter description here ...
   */
  public function mergedAttributes() {
    $attributes = array();
    if (isset($this->frxAttributes)) foreach ($this->frxAttributes as $key => $data) {
      FrxRenderer::addAttributes($attributes, $key, $data);
    }
    if (isset($this->htmlAttributes)) foreach ($this->htmlAttributes as $key => $data) {
      FrxRenderer::addAttributes($attributes, $key, $data);
    }
    $skin_data = Frx::Data()->getContext('skin');
    $class = get_class($this);

    if (isset($skin_data[$class])) {
      $attributes = array_merge($skin_data[$class], $attributes);
    }
    $classes = class_parents($this);
    array_pop($classes);
    if ($classes) foreach ($classes as $class) {
      if (isset($skin_data[$class])) {
        $attributes = array_merge($attributes, $skin_data[$class]);
      }
    }
    return $attributes;
  }

  public function replacedAttributes() {
    $attributes = array();
    if (isset($this->frxAttributes)) foreach ($this->frxAttributes as $key => $data) {
      $attributes[$key] =  $this->teng->replace((string)$data, TRUE);
    }
    if (isset($this->htmlAttributes)) foreach ($this->htmlAttributes as $key => $data) {
      $attributes[$key] = $this->teng->replace((string)$data, TRUE);
    }
    return $attributes;
  }

  /**
   * Return the inside xml of the current node
   *
   */
  public function innerXML() {
    $xml = $this->reportDocNode;
    $tag = $xml->getName();
    $text = '';
    if (is_object($xml) && is_object($xml->$tag)) {
      $text = $xml->asXML();
      $text = preg_replace("/<\/?" . $tag . "(.|\s)*?>/", "", $text);
    }
    return $text;
  }

  /**
   * Render a drupal form in a forena template
   * @param $form array
   */
  public function drupalRender($form) {
    $output = drupal_render($form);
    return $output;
  }


}