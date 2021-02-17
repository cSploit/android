---------------------------
Universal triggers (FB 1.5)
---------------------------

  Function:
    Allows triggers that handle multiple row operations.

  Author:
    Dmitry Yemanov <yemanov@yandex.ru>

  Syntax rules:
    CREATE TRIGGER name FOR table
      [ACTIVE | INACTIVE]
      {BEFORE | AFTER} <multiple_action>
      [POSITION number]
    AS trigger_body
    
    <multiple_action> ::= <single_action> [OR <single_action> [OR <single_action>]]
    <single_action> ::= {INSERT | UPDATE | DELETE}

  Example(s):
    1. CREATE TRIGGER TRIGGER1 FOR TABLE1 BEFORE INSERT OR UPDATE AS ...;
    2. CREATE TRIGGER TRIGGER2 FOR TABLE2 AFTER INSERT OR UPDATE OR DELETE AS ...;

  ODS:
    Encoding of field RDB$TRIGGER_TYPE (relation RDB$TRIGGERS) has been extended
    to allow complex trigger actions. Coding scheme is shown below (in C syntax):
    
    // trigger type prefixes
    #define TRIGGER_BEFORE		0
    #define TRIGGER_AFTER		1

    // trigger type suffixes
    #define TRIGGER_INSERT		1
    #define TRIGGER_UPDATE		2
    #define TRIGGER_DELETE		3

    // that's how trigger action types are encoded
	/*
    bit 0 = TRIGGER_BEFORE/TRIGGER_AFTER flag,
    bits 1-2 = TRIGGER_INSERT/TRIGGER_UPDATE/TRIGGER_DELETE (slot #1),
    bits 3-4 = TRIGGER_INSERT/TRIGGER_UPDATE/TRIGGER_DELETE (slot #2),
    bits 5-6 = TRIGGER_INSERT/TRIGGER_UPDATE/TRIGGER_DELETE (slot #3),
    and finally the above calculated value is decremented

    example #1:
        TRIGGER_AFTER_DELETE =
        = ((TRIGGER_DELETE << 1) | TRIGGER_AFTER) - 1 =
        = ((3 << 1) | 1) - 1 =
        = 0x00000110 (6)

    example #2:
        TRIGGER_BEFORE_INSERT_UPDATE =
        = ((TRIGGER_UPDATE << 3) | (TRIGGER_INSERT << 1) | TRIGGER_BEFORE) - 1 =
        = ((2 << 3) | (1 << 1) | 0) - 1 =
        = 0x00010001 (17)

    example #3:
        TRIGGER_AFTER_UPDATE_DELETE_INSERT =
        = ((TRIGGER_INSERT << 5) | (TRIGGER_DELETE << 3) | (TRIGGER_UPDATE << 1) | TRIGGER_AFTER) - 1 =
        = ((1 << 5) | (3 << 3) | (2 << 1) | 1) - 1 =
        = 0x00111100 (60)
    */

  Note(s):
    1. One-action triggers are fully compatible at ODS level with FB 1.0.
    2. RDB$TRIGGER_TYPE encoding is order-dependant, i.e.
       BEFORE INSERT OR UPDATE and BEFORE UPDATE OR INSERT will be coded differently,
       although they have the same semantics and will be executed exactly the same way.
    3. In multiple-action triggers both OLD and NEW contexts are available. If the
       trigger invocation forbids one of them (e.g. OLD context for INSERT operation),
       then all fields of that context will evaluate to NULL. If you assign to
       improper context, runtime exception will be thrown.
    4. You may use new context variables INSERTING/UPDATING/DELETING to check the
       operation type at runtime.

  See also:
    README.context_variables
