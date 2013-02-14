<?php
class FrxConfigData {
  public $access = 'access content';
  /**
   * Return XML List of defined repositories.
   */
  public function sources() {
    $data = Frx::RepoMan()->repositories;
    $node = new SimpleXMLElement('<repos/>');
    foreach ($data as $key=>$config) {
      $row = $node->addChild('row');
      $row->name = $key;
      $row->title = @$config['title'];
      $row->path = @$config['path'];
      $row->access_callback = @$config['access callback'];
    }
    return $node;
  }
}