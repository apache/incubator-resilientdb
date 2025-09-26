import { TITLE_MAPPINGS } from '@/lib/constants';
import { useQuery } from '@tanstack/react-query';

export interface Document {
  id: string;
  name: string;
  path: string;
  size: number;
  webViewLink:string;
  uploadedAt: string;
  displayTitle?: string;
}

function updateURL(url: string) {
  return url.substring(0, url.lastIndexOf('/')) + '/' + 'preview';
}
const getDisplayTitle = (filename: string): string => {
  const lowerFilename = filename.toLowerCase();
  return TITLE_MAPPINGS[lowerFilename] || filename.replace('.pdf', '');
};

function transformGroupedData(
  data: Record<string, any[]>,
  folderOrder: string[]
) {
  return folderOrder.map((folderId) =>
    (data[folderId] || []).map((doc) => ({
      ...doc,
      displayTitle: getDisplayTitle(doc.name),
      webViewLink: updateURL(doc.webViewLink),
    }))
  );
}


const fetchDocuments = async (): Promise<Document[][]> => {
  const response = await fetch('/api/research/documents');
  const data = await response.json();
  const folderIds = [
      "16yvY-jnMOZ1hMGaMYk0oi86eBbCqCHR2", // Class PDFs
      "1BKiGBrrrBMegCBUV1RlpkWBTktNPt4GL" // BOOKS
    ];
  
  if (!response.ok) {
    throw new Error(`Failed to fetch documents: ${response.status}`);
  }
  const transformed = transformGroupedData(data.data, folderIds);
  console.log(' documents transformed successfully',transformed)
  // console.log(Object.values(transformed).flat())
  return transformed; // Flatten the structure to return an array of documents
};



export const useDocuments = () => {
  return useQuery({
    queryKey: ['documents'],
    queryFn: fetchDocuments,
    staleTime: 5 * 60 * 1000, // 5 minutes
    gcTime: 10 * 60 * 1000, // 10 minutes
  });
}; 