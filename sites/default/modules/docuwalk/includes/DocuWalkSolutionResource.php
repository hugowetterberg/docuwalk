<?php
// $Id$

/**
 * Class that defines the solution resource
 *
 * @TargetedAction(name='add-waypoint', controller='addWaypoint')
 */
class DocuWalkSolutionResource {
  /**
   * Creates a solution
   *
   * @param object $solution ["data"]
   * @return object
   *
   * @Access(callback='DocuWalkSolutionResource::access', args={'create'}, appendArgs=false)
   */
  public static function create($solution) {
    global $user;
    $attr = array('title', 'body', 'task');
    $node = (object)array(
      'type' => 'docuwalk_solution',
      'created' => time(),
      'modified' => time(),
      'uid' => $user->uid,
    );

    // Transfer attributes from
    foreach ($attr as $name) {
      if (isset($solution->$name)) {
        $node->$name = $solution->$name;
      }
      else {
        return services_error("Missing attribute {$name}", 406);
      }
    }

    // Format the task nid for cck
    $node->field_docuwalk_task = array(
      array('nid' => $node->task),
    );
    unset($node->task);

    node_save($node);

    return (object)array(
      'nid' => $node->nid,
      'uri' => services_resource_uri(array('docuwalk-solution', $node->nid)),
      'url' => url('node/' . $node->nid, array('absolute' => TRUE))
    );
  }

  /**
   * Retrieves a solution
   *
   * @param int $nid ["path","0"]
   *  The nid of the solution to get
   * @return object
   *
   * @Access(callback='DocuWalkSolutionResource::access', args={'view'}, appendArgs=true)
   */
  public static function retrieve($nid) {
    return node_load($nid);
  }

  /**
   * Updates a solution
   *
   * @param int $nid ["path","0"]
   *  The nid of the solution to update
   * @param object $solution ["data"]
   *  The solution object
   * @return object
   *
   * @Access(callback='DocuWalkSolutionResource::access', args={'update'}, appendArgs=true)
   */
  public static function update($nid, $solution) {
    $attr = array('title', 'body');
    $node = node_load($nid);

    // Transfer attributes from
    foreach ($attr as $name) {
      if (isset($solution->$name)) {
        $node->$name = $solution->$name;
      }
    }

    node_save($node);

    return (object)array(
      'nid' => $node->nid,
      'uri' => services_resource_uri(array('docuwalk-solution', $node->nid)),
      'url' => url('node/' . $node->nid, array('absolute' => TRUE))
    );
  }

  /**
   * Deletes a solution
   *
   * @param int $nid ["path","0"]
   *  The nid of the solution to delete
   * @return bool
   *
   * @Access(callback='DocuWalkSolutionResource::access', args={'delete'}, appendArgs=true)
   */
  public static function delete($nid) {
    node_delete($nid);
  }

  /**
   * Retrieves a listing of solutions
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
    $parameters['type'] = 'docuwalk_solution';

    list($sql, $params) = $builder->query($fields, $parameters);
    $res = db_query_range($sql, $params, $page*20, 20);

    $nodes = array();
    while ($node = db_fetch_object($res)) {
      $node->uri = services_resource_uri(array('docuwalk-solution', $node->nid));
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
        'type' => 'docuwalk_solution',
      );
    }
    else if (!empty($args)) {
      $node = node_load($args[0]);
    }
    return node_access($op, $node);
  }

  /**
   * Controller method for writing waypoints to a solution
   *
   * @param int $nid ["path","0"]
   *  The nid of the solution to add the waypoint to
   * @param object $waypoint ["data"]
    *  The waypoint to add
   * @return object
   *
   * @Access(callback='DocuWalkSolutionResource::access', args={'update'}, appendArgs=true)
   */
  public static function addWaypoint($nid, $waypoint) {
    if (!isset($waypoint->position)) {
      return services_error("Missing attribute 'position'", 406);
    }

    if (!preg_match('/^-?\d+(\.\d+)?\s-?\d+(\.\d+)?$/', $waypoint->position)) {
      return services_error("Invalid format for the position.");
    }

    db_query("INSERT INTO {docuwalk_waypoint}(nid, position)
      VALUES(%d, GeomFromText('%s'))", $nid, simple_geo_to_wkt('point', $waypoint->position));
    $wid = db_last_insert_id('docuwalk_waypoint', 'wid');

    return (object)array(
      'wid' => $wid,
    );
  }
}