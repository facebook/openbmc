The BMC shall support memory post package repair operations if the memory and
the system support it.  The BMC shall initiate a procedure to remediate hard 
failing rows within a memory device that takes place during system POST. It
uses the spare rows to replace the broken row. The repair is permanent and
cannot be undone.  The details of the post package repair are documented in
the JEDEC [JESD79-5B][jedec] specification.

[jedec]: https://www.jedec.org/standards-documents/docs/jesd79-5b
