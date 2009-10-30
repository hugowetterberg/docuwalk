<?php
// $Id$

/**
 * Class that defines the text resource
 */
class DocuWalkTextResource {
  /**
   * Creates a text
   *
   * @param object $text ["data"]
   * @return object
   *
   * @Access(callback='DocuWalkTextResource::access', args={'create'}, appendArgs=false)
   */
  public static function create($text) {
    global $user;
    $attr = array('body', 'solution', 'position');
    $node = (object)array(
      'type' => 'docuwalk_text',
      'created' => time(),
      'modified' => time(),
      'uid' => $user->uid,
    );

    // Transfer attributes from
    foreach ($attr as $name) {
      if (isset($text->$name)) {
        $node->$name = $text->$name;
      }
      else {
        return services_error("Missing attribute {$name}", 406);
      }
    }

    $node->title = drupal_substr($node->body, 0, min(drupal_strlen($node->body), 40));

    // Format the solution nid for cck
    $node->field_docuwalk_solution = array(
      array('nid' => $node->solution),
    );
    unset($node->solution);

    $node->simple_geo_position = $node->position;
    unset($node->position);

    node_save($node);

    return (object)array(
      'nid' => $node->nid,
      'uri' => services_resource_uri(array('docuwalk-text', $node->nid)),
      'url' => url('node/' . $node->nid, array('absolute' => TRUE))
    );
  }

  /**
   * Retrieves a text
   *
   * @param int $nid ["path","0"]
   *  The nid of the text to get
   * @return object
   *
   * @Access(callback='DocuWalkTextResource::access', args={'view'}, appendArgs=true)
   */
  public static function retrieve($nid) {
    return node_load($nid);
  }

  /**
   * Updates a text
   *
   * @param int $nid ["path","0"]
   *  The nid of the text to update
   * @param object $text ["data"]
   *  The text object
   * @return object
   *
   * @Access(callback='DocuWalkTextResource::access', args={'update'}, appendArgs=true)
   */
  public static function update($nid, $text) {
    $attr = array('body');
    $node = node_load($nid);

    // Transfer attributes from
    foreach ($attr as $name) {
      if (isset($text->$name)) {
        $node->$name = $text->$name;
      }
    }

    $text->title = drupal_substr($text->body, 0, min(drupal_strlen($text->body), 40));

    node_save($node);

    return (object)array(
      'nid' => $node->nid,
      'uri' => services_resource_uri(array('docuwalk-text', $node->nid)),
      'url' => url('node/' . $node->nid, array('absolute' => TRUE))
    );
  }

  /**
   * Deletes a text
   *
   * @param int $nid ["path","0"]
   *  The nid of the text to delete
   * @return bool
   *
   * @Access(callback='DocuWalkTextResource::access', args={'delete'}, appendArgs=true)
   */
  public static function delete($nid) {
    node_delete($nid);
  }

  /**
   * Retrieves a listing of texts
   *
   * @param int $page ["param","page"]
   * @param string $fields ["param","fields"]
   * @param array $parameters ["param"]
   * @return array
   *
   * @Access(callback='user_access', args={'access content'}, appendArgs=false)
   */
  public static function index($page=0, $fields=array(), $parameters=array()) {
    $builder = new QueryBuilder();
    if ($params['__action']=='describe') {
      return $builder->describe();
    }

    if (is_string($fields)) {
      $fields = preg_split('/,\s?/', $fields);
    }
    $parameters['status'] = 1;
    $parameters['type'] = 'docuwalk_text';

    list($sql, $params) = $builder->query($fields, $parameters);
    $res = db_query_range($sql, $params, $page*20, 20);

    $nodes = array();
    while ($node = db_fetch_object($res)) {
      $node->uri = services_resource_uri(array('docuwalk-text', $node->nid));
      $nodes[] = $node;
    }
    return $nodes;
  }

  /**
   * Access checking function.
   *
   * @param string $op
   *  The operation that the user wants to perform.
   * @param array $args
   *  The arguments for the call.
   * @return bool
   *  True if access is given, otherwise false.
   */
  public static function access($op, $args) {
    if ($op=='create') {
      $node = (object)array(
        'type' => 'docuwalk_text',
      );
    }
    else if (!empty($args)) {
      $node = node_load($args[0]);
    }
    return node_access($op, $node);
  }
}