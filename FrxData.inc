<?php
/**
 * @file FrxData.inc
 * The FrxData class holds all of the data contexts during the report rendering process.
 * The general idea is that during the report render, data objects are pushed on the stack with the id's of the block or foreach objects that invoke them.
 * Rendering controls may then get current data contexts, and push data onto the stack or pop data onto the stack. They may also use this method to get the current data
 * context from the stack.
 *
 * The static instance function of this object acts as a factory method that allows other code in forena to always be able to operate on the current context of the class.
 * @var unknown_type
 */
require_once('FrxContext.inc');
class FrxData extends FrxContext {

  public $id;
  private $cur_context;  // The data of the xml;
  private $data_sources = array();
  private $data_stack = array();
  private $id_stack = array();

  // For backward compatibility.
  public static function instance() {
    return Frx::Data();
  }

  /**
   * Return the current data context
   *
   */
  public function currentContext() {
    return $this->cur_context;
  }

  /**
   * Return current context as key value pairs
   * Enter description here
   */
  public function currentContextArray() {
    $data = $this->cur_context;
    if (is_array($data)) {
      return $data;
    }

    $parms = array();
    if (is_object($data)) {
      // Get attributes
      $data = get_object_vars($data);
      if (method_exists($data, 'attributes')) {
        foreach ($data->attributes() as $key => $value) {
          $data[$key] = (string)$value;
        }
      }

      return $data;
    }

  }

  /**
   * Provides an api to the {=xpath} syntax that can be used
   * to evaluat expressions such as sum and count in a report.  We
   * need to use the DOM object here, because this method isn't exposed
   * with simplexml.
   *
   * @param $xml
   * @param $path
   * @return unknown_type
   */
  protected function simplexml_evaluate($xml, $path) {
    $dom_node = dom_import_simplexml($xml);
    $dom_doc = new DOMDocument('');
    $dom_node = $dom_doc->importNode($dom_node, TRUE);
    $dom_doc->appendChild($dom_node);
    // Do we also need to call AppendChild?
    $xpath = new DOMXpath($dom_doc);
    $ret = $xpath->evaluate($path, $dom_node);
    return $ret;
  }

  /**
   * Get the value from the data.
   * This is used by token_replace method to extract the data based on the path provided.
   * @param $data
   * @param $key
   * @return unknown_type
   */
  public function getValue($key, $context = '') {
    $retvar = '';
    // Default to theo current context
    $data = $this->currentContext();

    if ($context && $this->contextExists($context)) {
      $data = $this->getContext($context);
    }
    if (is_array($data)) {
      $retvar = @$data[$key];
    }
    elseif (is_object($data)) {

      if (strpos($key, '=')===0) {
        $retvar = $this->simplexml_evaluate($data, ltrim($key, '='));
      }
      else {

        $rows = @$data->xpath($key);
        if ($rows === FALSE) {
          drupal_set_message(t('Invalid field: "%s"', array('%s' => $key)), 'error', FALSE);
        }
        $x ='';
        if ($rows) $x = $rows[0];

        if ($x) $retvar = $x->asXML();
        // Check to see if there are child nodes
        // If so use asXML otherwise string cast.
        if ($retvar && strpos($retvar, '<')!==FALSE) {
          // Find the end of the first tag.
          $p = strpos($retvar, '>');
          $retvar = substr_replace($retvar, '', 0, $p+1);
          $p = strrpos($retvar, '<', -1);
          $retvar = substr_replace($retvar, '', $p, strlen($retvar) - $p);

        }
        else {
          $retvar = (string)$x;
        }
      }
    }

    if (!is_array($retvar)) $retvar = trim((string)$retvar);
    return $retvar;
  }

  /**
   * Allows override of a value for the current context.
   * @param String $key
   * @param String $value
   */
  public function setValue($key, $value, $context='') {
    if (is_array($this->cur_context)) {
      $this->cur_context[$key] = $value;
    }
    elseif (is_object($this->cur_context)) {
      if (strpos($key, '@')===0) {
        $this->cur_context[$key] = $value;
      }
      else {
        $this->cur_context->$key = $value;
      }
    }
  }

  /**
   * Push a data context onto the data stacks
   * to make sure that we can address these using an
   * appropriate syntax.  I think we don't need data_stack
   * but i'm holding it there in case we develop a "relative" path syntax.
   * @param $data
   * @param $id
   * @return unknown_type
   */
  public function push($data, $id='') {
    $this->data_stack[] = $this->cur_context;
    $this->id_stack[] = $this->id;
    $this->id = $id;
    $this->cur_context = $data;

    if ($id) {
      /*    if (@is_array($this->data_sources[$id]) && is_array($data)) {
       $data = array_merge($this->data_sources[$id], $data);
       }
       */
      $this->data_sources[$id] = $data;
    }
  }

  public function setContext($id, $data) {
    $this->data_sources[$id] = $data;
  }

  /**
   * Remove data from the data stack.  This will make data unavaiable
   * when we leave the context of the current nested reports.
   * @param $id
   * @return mixed
   */
  public function pop() {
    $this->id = array_pop($this->id_stack);
    $this->cur_context = array_pop($this->data_stack);
  }

  /**
   * Determines whether an array context exists for the specified id.
   * Returns true if the key exists othewise false
   * @param string $id
   * @return boolean
   */
  public function contextExists($id) {
    if (array_key_exists($id, $this->data_sources)) {
      return TRUE;
    }
    else return FALSE;
  }

  /**
   * Return a data context by id
   * Enter description here ...
   * @param unknown_type $id
   */
  public function getContext($id) {
    return @$this->data_sources[$id];
  }

  public function dumpContext() {
    drupal_set_message(filter_xss('cur_context ' . $this->id . '<br/>Stack<br/><pre>' . print_r($this->cur_context, 1) . '</pre>'));
  }




}