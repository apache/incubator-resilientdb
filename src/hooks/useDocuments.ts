import { TITLE_MAPPINGS } from '@/lib/constants';
import { useQuery } from '@tanstack/react-query';

export interface Document {
  id: string;
  name: string;
  path: string;
  size: number;
  uploadedAt: string;
  displayTitle?: string;
}

const fetchDocuments = async (): Promise<Document[]> => {
  const response = await fetch('/api/research/documents');
  
  if (!response.ok) {
    throw new Error(`Failed to fetch documents: ${response.status}`);
  }
  
  return response.json();
};

const getDisplayTitle = (filename: string): string => {
  const lowerFilename = filename.toLowerCase();
  return TITLE_MAPPINGS[lowerFilename] || filename.replace('.pdf', '');
};

export const useDocuments = () => {
  return useQuery({
    queryKey: ['documents'],
    queryFn: fetchDocuments,
    select: (data: Document[]) => 
      data.map((doc) => ({
        ...doc,
        displayTitle: getDisplayTitle(doc.name),
      })),
    staleTime: 5 * 60 * 1000, // 5 minutes
    gcTime: 10 * 60 * 1000, // 10 minutes
  });
}; 