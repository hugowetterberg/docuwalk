<?php
// $Id$

/**
 * Class that defines the task resource
 */
class DocuWalkTaskResource {
  /**
   * Creates a task
   *
   * @param object $task ["data"]
   * @return object
   *
   * @Access(callback='DocuWalkTaskResource::access', args={'create'}, appendArgs=false)
   */
  public static function create($task) {
    $attr = array('title', 'body');
    $node = (object)array(
      'type' => 'docuwalk_task',
      'created' => time(),
      'modified' => time(),
      'uid' => $user->uid,
    );

    // Transfer attributes from 
    foreach ($attr as $name) {
      if (isset($task->$name)) {
        $node->$name = $task->$name;
      }
      else {
        return services_error("Missing attribute {$name}", 406);
      }
    }

    node_save($node);

    return (object)array(
      'nid' => $node->nid,
      'uri' => services_resource_uri(array($type, $node->nid)),
      'url' => url('node/' . $node->nid, array('absolute' => TRUE))
    );
  }

  /**
   * Retrieves a task
   *
   * @param int $nid ["path","0"]
   *  The nid of the task to get
   * @return object
   *
   * @Access(callback='DocuWalkTaskResource::access', args={'view'}, appendArgs=true)
   */
  public static function retrieve($nid) {
    return node_load($nid);
  }

  /**
   * Updates a task
   *
   * @param int $nid ["path","0"]
   *  The nid of the task to update
   * @param object $task ["data"]
   *  The task object
   * @return object
   *
   * @Access(callback='DocuWalkTaskResource::access', args={'update'}, appendArgs=true)
   */
  public static function update($nid, $task) {
    $attr = array('title', 'body');
    $node = node_load($nid);

    // Transfer attributes from 
    foreach ($attr as $name) {
      if (isset($task->$name)) {
        $node->$name = $task->$name;
      }
    }

    node_save($node);

    return (object)array(
      'nid' => $node->nid,
      'uri' => services_resource_uri(array($type, $node->nid)),
      'url' => url('node/' . $node->nid, array('absolute' => TRUE))
    );
  }

  /**
   * Deletes a task
   *
   * @param int $nid ["path","0"]
   *  The nid of the task to delete
   * @return bool
   *
   * @Access(callback='DocuWalkTaskResource::access', args={'delete'}, appendArgs=true)
   */
  public static function delete($nid) {
    node_delete($nid);
  }

  /**
   * Retrieves a listing of tasks
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
    $parameters['type'] = 'docuwalk_task';
    
    list($sql, $params) = $builder->query($fields, $parameters);
    $res = db_query_range($sql, $params, $page*20, 20);

    $nodes = array();
    while ($node = db_fetch_object($res)) {
      $node->uri = services_resource_uri(array('docuwalk-task', $node->nid));
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
        'type' => 'docuwalk_task',
      );
    }
    else if (!empty($args)) {
      $node = node_load($args[0]);
    }
    return node_access($op, $node);
  }
}