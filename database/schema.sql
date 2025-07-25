-- postgresql schema for document index storage
-- this replaces the file-based storage in /documents/parsed/

-- document indices table: stores parsed document data and metadata
CREATE TABLE IF NOT EXISTS document_indices (
    document_path VARCHAR(500) PRIMARY KEY,
    parsed_data JSONB NOT NULL,
    metadata JSONB NOT NULL,
    original_file_size BIGINT NOT NULL,
    original_modified_time TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- index for faster queries on document paths
CREATE INDEX IF NOT EXISTS idx_document_indices_path ON document_indices(document_path);

-- index for faster queries on modification times (for cache validation)
CREATE INDEX IF NOT EXISTS idx_document_indices_modified_time ON document_indices(original_modified_time);

-- index for faster queries on parsed data content (gin index for jsonb)
CREATE INDEX IF NOT EXISTS idx_document_indices_parsed_data ON document_indices USING GIN (parsed_data);

-- function to automatically update the updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

-- trigger to automatically update the updated_at column
CREATE OR REPLACE TRIGGER update_document_indices_updated_at 
    BEFORE UPDATE ON document_indices 
    FOR EACH ROW 
    EXECUTE FUNCTION update_updated_at_column();

-- optional: document chunks table for more granular storage
-- this can be useful for advanced querying and analytics
CREATE TABLE IF NOT EXISTS document_chunks (
    chunk_id SERIAL PRIMARY KEY,
    document_path VARCHAR(500) NOT NULL REFERENCES document_indices(document_path) ON DELETE CASCADE,
    chunk_index INTEGER NOT NULL,
    text_content TEXT NOT NULL,
    chunk_metadata JSONB DEFAULT '{}'::jsonb,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- indexes for document chunks table
CREATE INDEX IF NOT EXISTS idx_document_chunks_path ON document_chunks(document_path);
CREATE INDEX IF NOT EXISTS idx_document_chunks_text ON document_chunks USING GIN (to_tsvector('english', text_content));
CREATE UNIQUE INDEX IF NOT EXISTS idx_document_chunks_unique ON document_chunks(document_path, chunk_index); 