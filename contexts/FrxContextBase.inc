<?php

/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/26/16
 * Time: 6:34 AM
 */

require_once('FrxContextInterface.inc');
abstract class FrxContextBase implements FrxContextInterface {
  
  /**
   * Return the properteis of the element. 
   * @return null|\SimpleXMLElement
   */
  public function asXML() {
    return FrxData::arrayToXml(get_object_vars($this));
  }
}