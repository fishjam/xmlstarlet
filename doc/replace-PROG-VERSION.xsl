<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:db="http://docbook.org/ns/docbook" version="1.0">

  <xsl:param name="VERSION"/>
  <xsl:template match="db:phrase[@role='VERSION']"><xsl:value-of select="$VERSION"/></xsl:template>

  <xsl:param name="PROG"/>
  <xsl:template match="db:phrase[@role='PROG']"><xsl:value-of select="$PROG"/></xsl:template>

  <!-- identity transform -->
  <xsl:template match="node()|@*">
    <xsl:copy><xsl:apply-templates select="node()|@*"/></xsl:copy>
  </xsl:template>

</xsl:stylesheet>
